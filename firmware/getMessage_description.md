# Подробное описание функции LoRaCom::getMessage

## Обзор
Функция `getMessage` является методом класса `LoRaCom`, ответственным за получение принятых LoRa пакетов из радиоинтерфейса. Она предоставляет простой API для неблокирующего приема пакетов, возвращая результат успешности и опциональную информацию о длине принятого пакета.

## Сигнатура функции
```cpp
bool LoRaCom::getMessage(char *buffer, size_t len, int* receivedLen)
```

## Параметры

### Входные параметры
- **`char *buffer`**: Указатель на символьный буфер, куда будут сохранены данные принятого пакета
  - Должен быть предварительно выделен вызывающим кодом
  - Размер должен вмещать ожидаемую максимальную длину пакета
- **`size_t len`**: Максимальный размер буфера в байтах
  - Предотвращает переполнение буфера
  - Должен соответствовать или превышать размер выделенного буфера
- **`int* receivedLen`** (опционально): Указатель для сохранения фактической длины принятого пакета
  - Может быть NULL, если информация о длине не нужна
  - Будет установлен в количество фактически принятых байтов при успешном приеме

## Возвращаемое значение
- **`bool`**: Возвращает `true`, если пакет был успешно принят, в противном случае `false`
  - `true`: Valid packet received and stored in buffer
  - `false`: No packet available, radio not initialized, or reception failure

## Detailed Implementation Analysis

### Обработка режима fake
Когда активен `isFakeMode` (для тестирования/симуляции):
```cpp
if (isFakeMode) {
    static int counter = 0;
    counter++;
    if (counter % 1000 == 0) {  // Симулируем периодический прием
        const char *fakeMsg = "Fake received message";
        if (strlen(fakeMsg) < len) {
            strcpy((char*)buffer, fakeMsg);
            actualLen = strlen(fakeMsg);
            if (receivedLen) *receivedLen = actualLen;
            return true;
        }
    }
    return false;
}
```
- Генерирует симулированные пакеты каждые 1000 вызовов для тестирования разработки
- Обеспечивает безопасность буфера проверкой длины сообщения
- Возвращает false, когда нет симулированного пакета

### Управление завершением передачи
После передачи пакета функция управляет переходами состояния радио:
```cpp
if (TxFinished && radioInitialised) {
    TxFinished = false;
    unsigned long txDuration = millis() - txStartTime;
    int state = radioUnion.sRadio->finishTransmit();

    #if DUTY_CYCLE_RECEPTION == 1
        // Для энергосбережения используем прием с duty cycle в стиле Meshtastic
        state |= radioUnion.sRadio->startReceiveDutyCycleAuto(MESHTASTIC_PREAMBLE_LENGTH, 8, MESHTASTIC_RADIOLIB_IRQ_RX_FLAGS);
    #else
        // Для тестирования трафика используем непрерывный прием
        state |= radioUnion.sRadio->startReceive();
    #endif

    TxMode = false;
    if (state == RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Tx завершен: %lu ms SF%d BW%.0f", txDuration, currentSF, currentBW);
    } else {
        ESP_LOGE(TAG, "Передача не удалась, код: %d", state);
    }
}
```
- Завершает операции передачи
- Переходит в режим приема в зависимости от конфигурации
- Поддерживает как duty-cycled, так и непрерывный режимы приема
- Ведет журнал статистики передачи

### Логика приема пакетов
Основная обработка приема:
```cpp
if (RxFlag && radioInitialised) {
    int state = radioUnion.sRadio->readData(reinterpret_cast<uint8_t *>(buffer), len);
    RxFlag = false;
    state |= radioUnion.sRadio->startReceive();  // Перезапуск приема
    bool result = (state == RADIOLIB_ERR_NONE);

    if (result) {
        // Разбор принятых данных
        actualLen = 0;
        for (size_t i = 0; i < len; i++) {
            if (buffer[i] == '\0') {
                actualLen = i;
                break;
            }
        }

        if (actualLen == 0) {  // Нулевой терминатора не найдено
            actualLen = len;  // Предполагаем полное использование буфера
        }

        if (receivedLen) *receivedLen = actualLen;
    }

    // Запуск WiFi POST при приеме LoRa пакета
    if (result && (POST_EN_WHEN_LORA_RECEIVED || force_lora_trigger) && wifi_manager_global) {
        ESP_LOGI(TAG, "Пакет LoRa принят, отправка POST триггера");
        ((WiFiManager*)wifi_manager_global)->setLoRaRssi(abs(radioUnion.sRadio->getRSSI()));
        ((WiFiManager*)wifi_manager_global)->setSendPostOnLoRa(true);
    }

    return result;
}
```
- Читает данные пакета с помощью API RadioLib
- Определяет фактическую длину пакета поиском нулевого терминатора
- Перезапускает режим приема для непрерывной работы
- Интегрируется с WiFi менеджером для пересылки пакетов
- Ведет журнал информации RSSI для принятых пакетов

## Сравнение с примером кода Meshtastic

### Прием пакетов в Meshtastic (из RadioLibInterface.cpp)
Прошивка Meshtastic предоставляет более сложную, готовую к продакшену реализацию приема:

```cpp
void RadioLibInterface::handleReceiveInterrupt()
{
    // Читаем длину пакета и данные
    size_t length = iface->getPacketLength();
    int state = iface->readData((uint8_t *)&radioBuffer, length);

    if (state != RADIOLIB_ERR_NONE) {
        LOG_ERROR("Ошибка приема пакета=%d", state);
        rxBad++;
    } else {
        // Обрабатываем допустимый пакет
        int32_t payloadLen = length - sizeof(PacketHeader);
        if (payloadLen < 0) {
            LOG_WARN("Пакет слишком короткий");
            rxBad++;
        } else {
            // Выделяем и заполняем MeshPacket
            meshtastic_MeshPacket *mp = packetPool.allocZeroed();
            mp->from = radioBuffer.header.from;
            mp->to = radioBuffer.header.to;
            mp->id = radioBuffer.header.id;
            // ... дополнительный разбор заголовка ...

            // Обрабатываем зашифрованный payload
            mp->which_payload_variant = meshtastic_MeshPacket_encrypted_tag;
            memcpy(mp->encrypted.bytes, radioBuffer.payload, payloadLen);
            mp->encrypted.size = payloadLen;

            // Передаем в прикладной уровень
            deliverToReceiver(mp);
        }
    }
}
```

### Ключевые отличия

1. **Сложность**: Meshtastic включает полный разбор пакетов, валидацию заголовков и маршрутизацию
2. **Структура пакетов**: Использует структурированные заголовки с source/destination/node ID
3. **Шифрование**: Обрабатывает зашифрованные payload'ы с protobuf кодированием
4. **Статистика**: Отслеживает хорошие/плохие RX для мониторинга
5. **Управление памятью**: Использует пулы пакетов для эффективного управления памятью
6. **Интеграция**: Глубокая интеграция с протоколами mesh сетей

### Упрощенный подход LoRaCom
- Фокус на получении сырых данных пакетов
- Минимальные накладные расходы протокола
- Подходит для простых приложений передачи данных
- Разработан для кастомных реализаций протоколов
- Легче использовать для базового тестирования и разработки

## Примеры использования

### Простой прием пакетов
```cpp
char buffer[256];
int receivedLen = 0;
bool hasPacket = loRaCom.getMessage(buffer, sizeof(buffer), &receivedLen);

if (hasPacket) {
    ESP_LOGI("Принятый пакет: %.*s (длина: %d)", receivedLen, buffer, receivedLen);
    // Обработка принятых данных...
}
```

### Непрерывный цикл приема
```cpp
void receiveLoop() {
    char buffer[256];
    while (true) {
        int len = 0;
        if (loRaCom.getMessage(buffer, sizeof(buffer), &len)) {
            // Обработка принятого пакета
            processPacket(buffer, len);
        }
        vTaskDelay(10 / portTICK_PERIOD_MS);  // Небольшая задержка
    }
}
```

## Рекомендации по обработке ошибок

1. **Размер буфера**: Убедитесь, что буфер достаточно велик для ожидаемых пакетов
2. **Нулевое завершение**: RadioLib может не завершать строки нулем
3. **Инициализация радио**: Функция корректно завершается, если радио не готово
4. **Потокобезопасность**: Функция не потокобезопасна; вызывающий должен синхронизировать доступ
5. **Управление памятью**: Время жизни буфера управляется вызывающим

## Характеристики производительности

- **Неблокирующая**: Возвращает немедленно, если пакеты недоступны
- **Низкая задержка**: Минимальная задержка обработки между ISR приема и доступностью данных
- **Эффективная память**: Без внутренней буферизации; использует буфер вызывающего
- **Дружелюбная к CPU**: Ограниченное использование CPU при отсутствии пакетов

## Замечания по интеграции

- Совместима с LoRa реализациями RadioLib
- Работает с сериями радио SX126x, SX127x, SX128x
- Настраивается через `lora_config.hpp`
- Интегрируется с WiFi менеджером для IoT приложений
- Поддержка как duty-cycled, так и непрерывных режимов приема

## Советы по отладке

1. Проверьте состояние `RxFlag` для активности ISR
2. Мониторьте `TxFinished` для правильного завершения передачи
3. Проверьте инициализацию радио с `radioInitialised`
4. Проверьте логи на ошибки передачи/приема
5. Проверьте размеры буферов и валидность указателей

## Связанные функции

- `sendMessage()`: Отправка пакетов
- `checkTxMode()`: Проверка состояния передачи
- `getRssi()`: Получение силы сигнала на приеме
- `setFrequency()`: Настройка частоты радио
- `setSpreadingFactor()`: Настройка параметров модуляции
