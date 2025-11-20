# Документация LoRa пакетов - приём и анализ

## Обзор
Документ описывает порядок вызовов функций при приёме LoRa пакета, получения уровня сигнала и подсчёта символов.

## Порядок получения LoRa пакета

### Шаг 0: Приёмные-events на СоC
- Местоположение: Радио-модуль (SX1262) в LoRa RX mode.
- События:
  - Обнаружение preamble (LoRa preamble sequence).
  - Sync Word matching (serviceInterval '2B').
  - Header received (packet length, flags).
  - Payload data received, CRC checked.
  - Если успешный, генерирует interrupt (DIO1 -> ESP32 ISR).
- ISR (`LoRaCom.cpp`): sets RxFlag = true, wakes task.

### Шаг 1: Мониторинг в основном цикле
- Местоположение: `Control::loRaDataTask()`
- Выполнено: опрос RxFlag, SPI чтение если flag set.
- Вызв функции:
  ```cpp
  void Control::loRaDataTask() {
      while (true) {
          if (m_LoRaCom->getMessage(buffer, sizeof(buffer))) {
              // Пакет получен
              // buffer содержит данные пакета
          }
      }
  }
  ```

### Шаг 2: Чтение данных от радиомодуля
- Местоположение: `LoRaCom::getMessage()`
- Вызовы:
  ```cpp
  bool LoRaCom::getMessage(char *buffer, size_t len) {
      // Проверка, что не fake mode
      if (!isFakeMode) {
          // Мониторинг RxFlag (ISR флаг приёма)
          if (RxFlag) {
              RxFlag = false;
              // Читать данные из радиу:
              size_t receivedLen = radioUnion.sRadio->readData((uint8_t*)buffer, len);
              buffer[receivedLen] = '\0';  // Добавить '\0'
              return true;
          }
      }
  }
  ```

- `radio->readData()`: копирует приёмные данные в buffer, возвращает количество байт (payload длина).
- Максимальный размер: `len = 128` (размер buffer).

### Шаг 3: Обработка в control
- После getMessage:
  ```cpp
  // Подсчёт длины
  int packetLen = strlen(buffer);
  // Логирование
  ESP_LOGI(TAG, "LoRa packet payload length: %d", packetLen);
  // Сохранение для POST
  m_wifiManager->setLastLoRaPacketLen(packetLen);
  ```

## Получение уровня сигнала пакета

### Текущая реализация
- Местоположение: `LoRaCom::getRssi()`
- Вызов: `radio->getRSSI()`
- Возвращает: текущее значение RSSI в dBm (миллисекунды, не per-пакет).
- Пример использования: в statusTask для включения RSSI в broadcast.

### Рекомендация для per-packet RSSI
Добавиться в `LoRaCom::getMessage()`:
```cpp
// После readData:
if (receivedLen > 0) {
    lastRssi = radio->getRSSI();  // Сохранить RSSI пакета
    // Возвращать local var или setter
}
```
Затем в control: `int rssi = m_LoRaCom->getLastRssi();`

## Подсчёт количества символов в пакете

### реализация
- Местоположение: `Control::loRaDataTask()`
- Вызов: `int packetLen = strlen(buffer);`
- Предварительный шаг: `buffer[receivedLen] = '\0'` в getMessage.
- Возвращает: количество payload байт (0-127, truncated если >127).
- Отличия:
  - Для неподдерживаемого payload >127: вернёт 127 (обрезанно).
  - CRC не учитывается (проверено radio, не копировано).
  - Для binary данных: передаёт байты, strlen() для text symbols.

### Ограничения
- Размер buffer: 128 bytes.
- LoRa max payload: зависит от SF/BW (обычно 256+ bytes).
- Fragmentation: НЕ поддерживаемо, пакеты <=127 приняты fully.
