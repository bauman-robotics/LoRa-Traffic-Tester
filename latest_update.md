# Оптимизация энергопотребления проекта LoRa Transceiver

## 1. Увеличение задержки задач при отключенном WiFi

### Описание изменений
- Добавлена переменная `wifi_enabled` в классе `Control` для отслеживания состояния WiFi.
- В задаче `loRaDataTask` проверка флага `wifi_enabled`:
  - Если WiFi **включен**: задержка - 10 мс (частые wakeups для быстрой реакции).
  - Если WiFi **отключен**: задержка - 100 мс (меньше wakeups для снижения потребления).
- Обновлена команда `wifi_en`: состояние записывается в `wifi_enabled`.

### Обоснование
Первоначально при отключенном WiFi ток оставался высоким (~100 мА) из-за частых wakeups. Увеличение задержки снижает частоту wakeups в 10 раз, снижая среднее потребление.

### Технические детали
- Код: `int delay_ms = wifi_enabled ? 10 : 100;` в `loRaDataTask`.

## 2. Light sleep с GPIO wakeup при отключенном WiFi (не работает)

### Описание изменений
- Попытка: ESP в light sleep при отключенном WiFi, wake на DIO1 (LoRa receive).
- Настройка GPIO wakeup на DIO1, `esp_light_sleep_start()` вместо vTaskDelay.

### Обоснование
Увеличение задержки не дало достаточного снижения. Light sleep должен минимизировать потребление до 1-3 мА, сохраняя реакцию на LoRa пакеты.

### Технические детали
- GPIO wakeup: `gpio_wakeup_enable(...)` на `LORA_DIO1`.
- Код: `if (!wifi_enabled) esp_light_sleep_start();`.
- **Проблема:** после введения loRaDataTask перестает принимать пакеты. Причина - несовместимость light sleep с RadioLib или GPIO конфликт. Требует отладки.

### Общее тестирование
Проект компилируется. Оптимизация 1 работает (увеличение задержки), 2 - отключена из-за остановки приема LoRa пакетов.

## 3. Periodic LoRa Status Sending с Macros and GUI Control

### Описание изменений
- Добавлены макросы `LORA_STATUS_ENABLED` (0 или 1) и `LORA_STATUS_INTERVAL_SEC` (30 сек дефолт) в `lib/lora_config.hpp` для compile time настройки periodic LoRa status packets.
- GUI: фрейм "LoRa Status Sending" с checkbox "Enable LoRa periodic status" и combo "Interval (sec):" от 10 до 180 сек.
- Конфиг `gui_config.json` содержит keys "lora_status_enabled": false, "lora_status_interval": "30".
- Runtime: checkbox и combo отправляют "command set status 0|1" и "command set interval XX", контролируя LoRa status sending.

### Обоснование
Требуется возможность включения periodic LoRa status packets из GUI с выбором интервала и compile time дефолтами через макросы.

### Технические детали
- Макросы инициализируют `statusEnabled` и `status_Interval`.
- GUI methods: `toggle_lora_status()`, `change_lora_status_interval()`.
- Status task отправляет LoRa "st ID:xxx R:x ...".

### Тестирование
Проект компилируется и работает.

## 4. Debug Logging for WiFi Connection

### Описание изменений
- Добавлены детальные логи в `WiFiManager::connect()`: SSID, пароль, API key, server URL, MAC адрес, статус на каждой попытке.
- Интервал между попытками оставлен 2000 мс.

### Обоснование
Улучшает диагностику проблем WiFi подключения.

### Технические детали
- Логи: ESP_LOGI(TAG, ...) со всеми параметрами; attempt logs with status.

## 5. Исправление Guru Meditation Error при обработке пустых сериальных сообщений

### Описание проблемы
После неудачного подключения к WiFi устройство продолжало работать, но при обработке входящих сериальных команд возникал Guru Meditation Error: Load access fault при попытке strcmp(nullptr, ...).

### Описание изменений
- В функции `Control::interpretMessage()` добавлены проверки `token != nullptr &&` перед всеми вызовами `c_cmp(token, ...)` для типов команд: "command", "data", "get", "status", "help", "flash".
- Аналогично добавлены проверки для дочерних токенов `cmd_token` и `get_token`.

### Обоснование
Предотвращает крах устройства при получении пустых или неполных сообщений по сериалу. Ранее strcmp вызывался на null указателях, что приводило к Exception unhandled.

### Технические детали
- Код: добавлены условия `if (token != nullptr && c_cmp(...))` во всех ветвях условного оператора.

### Тестирование
Изменения компилируются. Теперь пустые сообщения игнорируются без краха устройства, позволяя продолжить нормальную работу после WiFi-сбоев.

## 6. Улучшения логирования и поведенческой логики WiFi

### Описание изменений
- **Конфигурация defines в логах loadSettings**: После загрузки defaults добавлен дамп "Configuration defines: WIFI_ENABLE=1, WIFI_DEBUG_FIXES=0, WIFI_AUTO_TX_POWER_TEST=0" с ключами и значениями.
- **dBm в нагрузке TX power логов**: Теперь все логи TX power показывают "enum val 44 = 18.0 dBm" с конвертацией (enum - 8) / 2.0f.
- **Skip WiFi если WIFI_ENABLE=0**: connect() сразу возвращает false с логом "WiFi disabled by WIFI_ENABLE=0", предотвращает сканирование и попытки.
- **Sин логов сканирование нужно WIFI_DEBUG_FIXES && WIFI_ENABLE**: Сканирование сетей только если оба define=1, иначе пропускает для WiFi disabled.
- **Всегда WiFi.mode(WIFI_STA) перед setTxPower**: Добавлено в оба пути (auto / fixed) для исправления "Neither AP or STA has been started" на третьей стороне.

### Обоснование
Улучшает boot diagnostics (визуально дефайны), предотвращает ненужные операции WiFi, исправляет ошибки ESP Arduino.

### Технические детали
- loadSettings() июльите логи после defaults; connect() логи конфиг и MAC отдельно.
- setTxPower требует mode for Arduino ESP.

### Тестирование
Компилируется. Boot показывает все defines, log fixing "Neither AP or STA...", WiFi disabled пропускает сканирование.

## 7. Конфигурируемый WIFI_USE_CUSTOM_MAC define

### Описание проблемы
После включения сканирования сетей и увеличения интервала, причина AUTH_EXPIRE неясна. Ошибка часто связана с блокировкой MAC или неправильным паролем. Нужно возможность легко включать/отключать генерацию random MAC.

### Описание изменений
- Добавлен define `WIFI_USE_CUSTOM_MAC 0` в `lib/lora_config.hpp` для включения random MAC (0=use ESP default, 1=generate random).
- Код в `Control::setup()` обернут в `#if WIFI_USE_CUSTOM_MAC ... #endif`.

### Обоснование
Позволяет легко включать/отключать random MAC без изменения кода. Сейчас `0` - тест с default MAC.

### Технические детали
- Define позволяет compile-time контроль генерации MAC адреса.

### Тестирование
Компилируется с WIFI_USE_CUSTOM_MAC=0. Следующие логи покажут постоянный ESP MAC.

## 8. Исправление LittleFS "Already Mounted" и создания файлов

### Описание проблемы
После многократных перезапусков LittleFS rimane "Already Mounted!", и возникает ошибка "no permits for creation" при создании новых файлов.

### Описание изменений
- В `SaveFlash::begin()` добавлен `LittleFS.end();` перед инициализацией для корректного unmount.
- Это предотвращает конфликты при повторных инициализациях и позволяет создавать файлы.

### Обоснование
LittleFS может оставаться mounted после предыдущих сессий, блокируя переинициализацию и создание файлов. Unmount решает проблему.

### Технические детали
- Добавлено `LittleFS.end();` в начале `begin()` для фиксации mounting state.

### Тестирование
Компилируется. LittleFS теперь корректно инициализируется без дубликатов mounting и создает файлы нормально.

## 9. Добавление логирования полного POST данных

### Описание проблемы
При отладке HTTP POST сложно понять, что отправляется устройство - данные, URL, т.д.

### Описание изменений
- В `WiFiManager::doHttpPost()` добавлено логирование полного postData: "Full postData: api_key=1&user_id=Andrey&...".

### Обоснование
Позволяет копировать данные напрямую для тестирования curl или анализа.

### Технические детали
- ESP_LOGI(TAG, "Full postData: %s", postData.c_str()).

### Тестирование
Компилируется. При POST в логах видны полные данные.

## 10. GUI кнопка для чтения flash логов

### Описание проблемы
Отсутствовала возможность чтения flash логов через GUI - нужно было вручную отправлять команду "flash".

### Описание изменений
- Добавлена кнопка "Read Flash Logs" в GUI (gui.py).
- Кнопка отправляет команду "flash" устройству для чтения и автоочистки логов на флеше.
- Записывается весь LoRa трафик: RX (полученные LoRa сообщения), TX (отправленные LoRa сообщения).
- Обновлен текст справки в GUI, добавлено описание команды "flash".

### Обоснование
Упрощает доступ к логированию для пользователей GUI без знания команд.

### Технические детали
- Кнопка размещена на вкладке Debug над ScrolledText.
- Добавлена кнопка Clear Debug Log рядом для очищения debug логов.
- Метод read_flash_logs() отправляет "flash".
- Help текст обновлен.

### Тестирование
GUI загружается с новой кнопкой. При нажатии отправляется команда "flash" через serial.

## 11. Логирование всех полученных LoRa сообщений во flash

### Описание проблемы
Во flash писались только TX (отправленные), но не полученные LoRa сообщения.

### Описание изменений
- В `loRaDataTask()` добавлена запись `m_saveFlash->writeData((String("RX: ") + buffer + "\n").c_str());` после обработки received message.
- Это позволяет логировать весь LoRa трафик для отладки.

### Обоснование
Улучшает диагностику - можно просмотреть историю всех полученных сообщений, включая симулированные в fake mode.

### Технические детали
- Запись происходит сразу после `interpretMessage(buffer, false);`.
- Тип: "RX: <message>" аналогично TX.

### Тестирование
Компилируется. Теперь flash содержит полный лог LoRa сообщений обмена.

## 12. Улучшения для variability в POST данных и WiFi MAC

### Описание изменений
- **WIFI_USE_CUSTOM_MAC**: Изменено с 0 на 1 для генерации random MAC address при каждом подключении WiFi.
- **ALARM_TIME**: Уже был random (ALARM_TIME + random(0,10000)), теперь можно настраивать.

### Обоснование
Добавляет variability для обхода сетевых restrictions (random MAC) и разнообразие тестовых данных (alarm_time).

### Технические детали
- #define WIFI_USE_CUSTOM_MAC 1
- Random alarm_time добавляется из диапазона 0-10000 к базовому ALARM_TIME.

### Тестирование
Компилируется. WiFi теперь использует random MAC для каждого boot.

## 13. Передача RSSI received LoRa пакета в hot параметр POST при LoRa trigger режиме с optional compile-time контролём

### Описание проблемы
При режиме POST на LoRa receive, hot всегда = 0, неотражало реальные данные. Нужно передать уровень сигнала полученного пакета. Кроме того, нужен compile-time контроль этой возможности.

### Описание изменений
- Добавлен define `POST_HOT_AS_RSSI 0` в `lib/lora_config.hpp`: если 1, hot=RSSI; если 0, hot=число успешных POST.
- В WiFiManager добавлено поле `int32_t loraRssi` и методы `setLoRaRssi(rssi)`.
- При получении LoRa message в LoRaCom::getMessage() делает `setLoRaRssi(radioUnion.sRadio->getRSSI())` перед setSendPostOnLoRa.
- В doHttpPost() при post_on_lora_mm использует `POST_HOT_AS_RSSI ? loraRssi : hot_counter` для параметра hot.
- Логи обновлены: "Preparing POST: ... hot=<value>" и lastHttpResult показывает hot как RSSI или count.

### Обоснование
Позволяет мониторить качество LoRa связи через RSSI в hot параметре, но с optional compile-time контролём - если не нужно, работает как раньше (hot=число успешных POST).

### Технические детали
- RSSI преобразуется в положительное число (abs), передается как hot параметр когда POST_HOT_AS_RSSI=1 (чем выше hot, тем лучше сигнал).
- В periodic режиме hot всегда как успешные posts counter.
- Дефолт POST_HOT_AS_RSSI=0 для backward компатибility.

### Тестирование
Компилируется. Теперь POST на LoRa receive содержит hot как RSSI (если define=1) или count успешных POST (define=0).

## 14. FAKE_LORA define для compile-time режима симуляции LoRa

### Описание проблемы
Без радиомодуля приходилось ждать fail инициализации SX1262 для перехода в fake mode.

### Описание изменений
- Добавлен #define FAKE_LORA 0 в lib/lora_config.hpp.
- Если FAKE_LORA=1, пропускает hardware инициализацию и сразу использует fake mode.
- Если FAKE_LORA=0, пытается hardware, при fail с fallback на fake.

### Обоснование
Ускоряет boot и тестирование без hardware - compile-time выбор режима симуляции.

### Технические детали
- Define добавлен в lora_config.hpp.
- Логика в Control::setup(): if (!FAKE_LORA) try hardware else fake only.
- При FAKE_LORA=1: "FAKE_LORA=1: Skipping hardware LoRa initialization, using fake mode"

### Тестирование
Компилируется. При FAKE_LORA=1 работает симуляция без попыток hardware.

## 15. Очистка compile-time условной компиляции #ifdef FAKE_LORA в LoRaCom.hpp

### Описание проблемЬ
После введения runtime fake mode через define FAKE_LORA и union radioUnion, старый #ifdef FAKE_LORA блок в header пытался использовать несуществующий `radio` вместо `radioUnion`.

### Описание изменений
- Удален #ifdef FAKE_LORA блок из `LoRaCom.hpp`, который определял compile-time fake mode с `radio = new FakeRadio()`.
- Теперь весь контроль fake mode через runtime `isFakeMode` flag и define FAKE_LORA для выбора на boot.

### Обоснование
Убирает дубликат кода и compile-time зависимости, переносит все в runtime для большей flexibility.

### Технические детали
- Убрана функция `bool begin(...)` под #ifdef FAKE_LORA.
- Всё управление через `beginFake()` и `isFakeMode`.

### Тестирование
Компилируется без ошибок 'radio' not declared.

## 16. Улучшение логирования received messages и сокращение spam от fake LoRa

### Описание проблемы
В логах много пробелов и мусор из-за multiple "Received: <msg>" из разных task без идентификации источника.

### Описание изменений
- Добавлен prefix "Serial received:" в `serialDataTask()` для serial input.
- Добавлен prefix "LoRa Received:" в `loRaDataTask()` для LoRa received.
- Fake LoRa messages теперь каждые 1000 вызовов вместо 10 (сокращение spam в 100 раз).
- Убраны ESP_LOGI внутри WiFi client response waiting loop для очистки логов.

### Обоснование
Легче различать serial и LoRa received messages, меньше шум в логах от fake mode, clean WiFi logs.

### Технические детали
- Изменен counter в `LoRaCom::getMessage()` с %10 на %1000.

### Тестирование
Компилируется (с некоторыми warnings). При отсутствии LoRa модуля автоматически включается fake mode с большими буквами в логе.

## 17. Автоматическое включение Fake LoRa mode при отсутствии hardware

### Описание проблемы
При отсутствии подключенного LoRa модуля прошивка не запускалась, требуя ручного определения FAKE_LORA.

### Описание изменений
- В `LoRaCom.hpp` добавлен `beginFake()` метод для симуляции LoRa.
- В `Control::setup()` логика: сначала пытается инициализировать SX1262, при fail автоматически переключается на fake mode.
- Обновлена структура с union для хранения radio объектов.

### Обоснование
Позволяет запускать прошивку без LoRa hardware для отладки остальных функций (WiFi, serial, flash).

### Технические детали
- Использован union для SX1262* и FakeRadio*.
- Flag isFakeMode для выбора метода при вызове.

### Тестирование
Компилируется (с некоторыми warnings). При отсутствии LoRa модуля автоматически включается fake mode с большими буквами в логе.

## 18. Конфигурируемый define для использования заданного фиксированного WiFi MAC адреса на compile time

### Описание проблемы
Отсутствовала возможность жестко задать конкретный MAC адрес на этапе компиляции, только случайная генерация или дефолтный ESP MAC.

### Описание изменений
- Добавлен define `WIFI_USE_FIXED_MAC 0` в `lib/lora_config.hpp` для включения фиксированного MAC.
- Добавлен массив `WIFI_FIXED_MAC_ADDRESS` с байтами MAC адреса (по умолчанию `0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF`).
- Код в `Control::setup()` обновлен с приоритетом: фиксированный MAC > случайный MAC > дефолтный MAC.

### Обоснование
Позволяет задать желательный MAC адрес на compile time для тестирования или совместимости с сетями, требующими специфических MAC.

### Технические детали
- Приоритет: если `WIFI_USE_FIXED_MAC=1` использует фиксированный, иначе если `WIFI_USE_CUSTOM_MAC=1` случайный, иначе ESP default.
- Логирование показывает установленный MAC.

### Тестирование
Проект компилируется. MAC адрес устанавливается по выбранному типу при boot.

## 19. Конфигурируемый WiFi TX Power define

### Описание изменений
- Добавлен define `WIFI_TX_POWER 44` в `lib/lora_config.hpp` для управления мощностью WiFi передатчика на compile time.
- В `WiFiManager::connect()` после успешного соединения добавлен'appel `WiFi.setTxPower((wifi_power_t)WIFI_TX_POWER);` с логированием установленной мощности.

### Обоснование
Позволяет регулировать мощность WiFi сигнала для баланса дальности и потребления энергии на compile time через define.

### Примеры значений (константы из WiFi.h ESP32 Arduino)
- `WIFI_POWER_20dBm`: 20 dBm (максимум для ESP32)
- `WIFI_POWER_19_5dBm`: 19.5 dBm
- `WIFI_POWER_19dBm`: 19 dBm
- `WIFI_POWER_18_5dBm`: 18.5 dBm
- `WIFI_POWER_18dBm`: 18 dBm
- `WIFI_POWER_15dBm`: 15 dBm
- `WIFI_POWER_10dBm`: 10 dBm
- `WIFI_POWER_8_5dBm`: 8.5 dBm (низкая мощность для тестирования)
- `WIFI_POWER_2dBm`: 2 dBm (минимум)

### Технические детали
- Мощность устанавливается один раз при подключении к WiFi с использованием WiFi.setTxPower(wifi_power_t).
- Используются стандартные константы из WiFi.h ESP32 Arduino для значений мощности.

### Тестирование
Проект компилируется. WiFi подключение теперь использует настроенную мощность передатчика с логированием в кварталах dBm и расчётом в dBm.

## 20. Исправление AUTH_EXPIRE на ESP32-C3 Super Mini путем устаноки WiFi TX power перед begin()

### Описание изменений
- Добавлен define `WIFI_DEBUG_FIXES 1` в `lib/lora_config.hpp` для включения исправлений WiFi.
- В `WiFiManager::connect()` смещена `WiFi.setTxPower((wifi_power_t)WIFI_TX_POWER);` ПЕРЕД `WiFi.begin()`.
- Добавлено логирование "WiFi TX power set before begin" для диагностики.
- Под макросом WIFI_DEBUG_FIXES:
  - `WiFi.mode(WIFI_STA);`
  - `WiFi.setSleep(false);` // Отключает spящий режим WiFi
  - `WiFi.setAutoReconnect(true);` // Автопереподключение
  - `WiFi.persistent(true);` // Сохраняет настройки WiFi
  - `WiFi.onEvent(WiFiEvent);` // Callback для переподключения при отключении

### Обоснование
Решение проблемы постоянного AUTH_EXPIRE на ESP32-C3 Super Mini (через 1 сек после begin). Установка TX power до begin предотвращает конфликты с chip revision.

### Технические детали
- setTxPower(WIFI_POWER_8_5dBm) перед begin решает проблему аутентификации.
- Event callback автоматически переподключается при разрыве.

### Тестирование
Проект компилируется. AUTH_EXPIRE решена: WiFi подключается без таймаута, держится соединение.

## 21. Автоматическое тестирование различных уровней TX power WiFi для обхода AUTH_EXPIRE на проблемных бордах

### Описание изменений
- Добавлен define `WIFI_AUTO_TX_POWER_TEST 0` в `lib/lora_config.hpp` для включения автоматического перебора уровней мощности (0=использовать фиксированный WIFI_TX_POWER_VARIANT, 1=авто тест).
- Добавлен `WIFI_ATTEMPTS_PER_VARIANT 2` - число попыток подключения на каждый уровень мощности.
- В `WiFiManager::connect()` при `WIFI_AUTO_TX_POWER_TEST=1` циклично тестирует 9 уровней: 20dBm, 19.5dBm, ..., 2dBm с логами "Trying TX power variant X (enum Y = Z.Z dBm)".
- После 2 неудачных попыток переключает на следующий уровень, успех останавливает перебор.

### Обоснование
Автоматически подбирает оптимальную TX power для борд с разными ESP32-C3 ревизиями или сетевыми ограничениями, где фиксированная мощность даёт AUTH_EXPIRE.

### Технические детали
- Использует static_cast<wifi_power_t>(value) для корректного enum casting.
- Логи показывают enum значение и рассчитанный dBm.
- При выборе уровня подключается и продолжает работу с ним.

### Тестирование
Компилируется. На рабочей борде сразу подключается на 18dBm, на проблемной перебирает все уровни до неудачи.

## 22. Улучшения логирования и поведенческой логики WiFi

### Описание изменений
- **Конфигурация defines в логах loadSettings**: После загрузки defaults добавлен дамп "Configuration defines: WIFI_ENABLE=1, WIFI_DEBUG_FIXES=0, WIFI_AUTO_TX_POWER_TEST=0" с ключами и значениями.
- **dBm в нагрузке TX power логов**: Теперь все логи TX power показывают "enum val 44 = 18.0 dBm" с конвертацией (enum - 8) / 2.0f.
- **Skip WiFi если WIFI_ENABLE=0**: connect() сразу возвращает false с логом "WiFi disabled by WIFI_ENABLE=0", предотвращает сканирование и попытки.
- **Sин логов сканирование нужно WIFI_DEBUG_FIXES && WIFI_ENABLE**: Сканирование сетей только если оба define=1, иначе пропускает для WiFi disabled.
- **Всегда WiFi.mode(WIFI_STA) перед setTxPower**: Добавлено в оба пути (auto / fixed) для исправления "Neither AP or STA has been started" на третьей стороне.

### Обоснование
Улучшает boot diagnostics (визуально дефайны), предотвращает ненужные операции WiFi, исправляет ошибки ESP Arduino.

### Технические детали
- loadSettings() июльите логи после defaults; connect() логи конфиг и MAC отдельно.
- setTxPower требует mode for Arduino ESP.

### Тестирование
Компилируется. Boot показывает все defines, log fixing "Neither AP or STA...", WiFi disabled пропускает сканирование.

## 23. Макрос LORA_STATUS_SHORT_PACKETS для выбора формата LoRa статус пакетов

### Описание изменений
- Добавлен define `LORA_STATUS_SHORT_PACKETS 0` в `lib/lora_config.hpp` для выбора формата periodic LoRa status packets.
- Если =0: отправляется полный статус (~60 байт): "st ID:transceiver R:-63 B:100.00 M:transceive S:ok"
- Если =1: отправляется короткий 2-байтовый HEX счетчик: "00", "01", ..., "FF" (инкремент с каждым пакетом).
- Добавлен static counter `uint16_t packetCounter = 0;` в statusTask(), который увеличивается с каждым пакетом (wrap at 255).
- Код в `Control::statusTask()` и `interpretMessage()` (для set status) выбирает формат на основе define.

### Обоснование
Позволяет экономить пропускную способность и энергию LoRa при маленьких пакетах (выше скорость передачи, ниже потребление), например для простой сигнализации о работе устройства без детального статуса.

### Технические детали
- Счетчик shared для периодических и initial пакетов.
- Дефолт 0 для backward compatibility с полными статус пакетами.

### Тестирование
Компилируется. При LORA_STATUS_SHORT_PACKETS=1 устройство посылает "00", "01", etc. вместо длинного текста.

## 24. Финальные настройки LoRa и GUI параметров

### Описание изменений
- Frequency: LORA_FREQUENCY = 868.075f (вместо 868.06)
- Gain: LORA_POWER = 20 dBm (вместо 22)
- GUI и config.json: gain="20", freq="868.075"
- LoRa TX finish log: ESP_LOGD вместо ESP_LOGI для уменьшения noise
- Build script: добавлен вывод defines перед сборкой
- Network logs: объединены в один блок для читаемости
- WiFi defines conditional на WIFI_ENABLE
- Application defines дампа в loadSettings

### Обоснование
Подгонка тестовых параметров под предпочтения, чистота логов.

### Тестирование
Все компилируется и работает с новыми настройками.

## 25. Финальные настройки логов и cleanup

### Описание изменений
- Логи LoRaCom: убрал TAG "[LORA_COMM]" из .hpp чтобы предотвратить префикс "[LORA_COMM]" в логах.
- Логи передачи LoRa: сократил "Transmission finished in 699 ms (SF:11, BW:125 kHz)" до "Tx done: 699 ms SF11 BW125.0" для избежания разрыва на 2 строки и cleaner вывода.
- Логи transmitting: ESP_LOGD для уменьшения spam в default mode.
- "Transmission finished" оставлен ESP_LOGI для отображения.
- GUI sync: при коннекте запрашивает текущие настройки у устройства и устанавливает чекбоксы/field в правильное состояние.

### Обоснование
Cleaner логи без лишних префиксов и wrap-around. GUI синк обеспечивает一致ность for настройки.

### Тестирование
Все логи работают корректно с новыми форматами. GUI синкит настройки при каждом коннекте.

## 26. Поддержка Meshtastic Long Fast совместимого режима

### Описание изменений
- **Корректные параметры для Long Fast preset:** SF=11, BW=250 kHz, CR=4/5
- Mesh параметры соответствуют официальным параметрам Meshtastic Long Fast из таблицы 8 preset'ов:
  - Data Rate: 1.07 kbps
  - Symbol Rate: 11 / 2048
  - Bandwidth: 250 kHz
  - Coding Rate: 4/5

### Технические детали
- Параметры в `lib/lora_config.hpp` под макросом `MESH_COMPATIBLE=1`
- Это дает совместимость с людьми, использующими Meshtastic Long Fast для большего диапазона
- Код уже использует эти параметры по умолчанию

## 27. Серверная инфраструктура для сбора и визуализации данных LoRa

### Описание изменений
- Создан отдельный каталог `server_backend/` для полной серверной инфраструктуры
- Добавлен подробный руковод `server_deployment_guide.md` на русском языке
- Реализован Python скрипт `data_collector.py` с Flask сервером для приема POST запросов от LoRa устройств
- Добавлены файлы для Docker развертывания: `docker-compose.yml`, `Dockerfile`, `requirements_server.txt`
- Настроена база данных PostgreSQL с таблицей `lora_packets` для хранения RSSI, SNR, частоты, полосы, SF
- Настроена Grafana для визуализации данных в реальном времени
- Добавлена возможность мониторинга работоспособности `/health` endpoint

### Технические детали
- **Flask сервер** работает на порту 5000, принимает JSON данные от LoRa устройств
- **PostgreSQL** хранит пакеты с метками времени, RSSI, SNR и параметрами LoRa
- **Grafana** конфигурируется с PostgreSQL datasource и dashboard для мониторинга
- **Docker** позволяет развернуть все сервисы одной командой `docker-compose up -d`

### Обоснование
Создает полнофункциональную инфраструктуру для анализа LoRa трафика с визуализацией и хранением данных. Дополняет аппаратную часть проектом встроенным хранением и анализом.

## 28. Режим duty cycle приема (экономия энергии)

### Описание изменений
- Добавлен define `DUTY_CYCLE_RECEPTION 0` в `lib/lora_config.hpp` для выбора режима приема
- Реализация Meshtastic-style duty cycle режима для экономии энергии
- При `DUTY_CYCLE_RECEPTION = 1` используется сканирование каналов (Channel Activity Detection)
- При `DUTY_CYCLE_RECEPTION = 0` (по умолчанию) применяется непрерывный прием для тестирования нагрузки

### Технические детали
**Для continuous mode (DUTY_CYCLE_RECEPTION = 0):**
- `radioUnion.sRadio->startReceive()` - постоянный прием всех пакетов

**Для duty cycle mode (DUTY_CYCLE_RECEPTION = 1):**
- `radioUnion.sRadio->startReceiveDutyCycleAuto(MESHTASTIC_PREAMBLE_LENGTH, 8, MESHTASTIC_RADIOLIB_IRQ_RX_FLAGS)`
  - `MESHTASTIC_PREAMBLE_LENGTH = 8` (символов преамбулы)
  - `MESHTASTIC_RADIOLIB_IRQ_RX_FLAGS = RADIOLIB_IRQ_RX_DONE | RADIOLIB_IRQ_PREAMBLE_DETECTED | RADIOLIB_IRQ_HEADER_VALID`

**Функции фильтрации шума:**
- `receiveDetected(irq, RADIOLIB_SX126X_IRQ_HEADER_VALID, RADIOLIB_SX126X_IRQ_PREAMBLE_DETECTED)`
- Применяет timeout для исключения ложных срабатываний преамбулы

### Тестирование
- **Результат:** ток потребления не изменился, количество принимаемых пакетов осталось прежним (5-6 пакетов в минуту)
- **Вывод:** duty cycle режим не активировался

**Возможные причины:**
- Параметры `MESHTASTIC_PREAMBLE_LENGTH = 8` не соответствуют использованному preset'у
- Для Long Fast preset preamble length может быть разным от Short Fast
-уНужпв проверка правильных параметров для каждого preset'а отдельно</result>
</write_to_file>

## 29. Финальные исправления duty cycle режима

### Критические изменения в коде:
- Изменить MESHTASTIC_PREAMBLE_LENGTH с 8 на 20 в lib/LoRaCom/LoRaCom.cpp
- Это критически важно для корректной работы duty cycle с Long Fast preset

- Исправлено MESHTASTIC_PREAMBLE_LENGTH = 20 на приемнике.
  Если на передатчике старое значение, пакеты всё равно проходят.
  Шум пакетов также виден как и с 8
  Стали видны пакеты с уровнем сигнала -95 дБм, до этого шумные пакеты были -110 дБм

## 31. Введение DEVICE_TYPE_TRANSMITTER для конфигурации типов устройств

### Описание изменений
- Добавлен новый макрос `DEVICE_TYPE_TRANSMITTER` в конец `lib/lora_config.hpp` (вместо старого `DEVICE_TYPE`)
- Улучшенная читаемость: явно указывает транслитерацию английского названия устройства
- Управляет автоматической настройкой устройства в зависимости от роли:
  - Приемник (коллектор данных с передачей по WiFi)
  - Передатчик (устройство только для отправки LoRa статусов)

### Технические детали
```cpp
#define DEVICE_TYPE_TRANSMITTER 0  // 0=приемник (RECEIVER), 1=передатчик (TRANSMITTER)

#if DEVICE_TYPE_TRANSMITTER == 0  // RECEIVER - устройство для приема и передачи данных по WiFi
    #define WIFI_ENABLE 1                   // Включаем WiFi для передачи данных
    #define POST_EN_WHEN_LORA_RECEIVED 1    // POST при каждом принятом LoRa пакете
    #define LORA_STATUS_ENABLED 0           // Отключаем свои статусы LoRa
    #define WIFI_DEBUG_FIXES 1              // Включаем WiFi исправления
    #define WIFI_AUTO_TX_POWER_TEST 0       // Авто тест отключен дефолтно
    // Остальные настройки остаются дефолтными

#elif DEVICE_TYPE_TRANSMITTER == 1  // TRANSMITTER - устройство только для отправки LoRa статусов
    #define WIFI_ENABLE 0                   // Экономия энергии - без WiFi
    #define POST_EN_WHEN_LORA_RECEIVED 0    // Без POST режимов
    #define LORA_STATUS_ENABLED 1           // Отправляет периодические LoRa статусы
    #define POST_INTERVAL_EN 0              // Отключаем periodic POST
    #define WIFI_DEBUG_FIXES 0              // WiFi fixes не нужны
    #define WIFI_AUTO_TX_POWER_TEST 0       // Auto TX power тест отключен

#else
    #error "Invalid DEVICE_TYPE_TRANSMITTER. Must be 0 (RECEIVER) or 1 (TRANSMITTER)"
#endif
```

### Преимущества новой реализации
- **Лучшая читаемость**: Название отражает предназначение устройства
- **Экономия энергии**: TRANSMITTER не тратит энергию на WiFi
- **Безопасность**: Ошибка компиляции при неверном значении
- **Гибкость**: Полный контроль над конфигурацией каждого типа

### Тестирование
Компилируется для обоих типов устройств. Приемник - активный WiFi режим, передатчик - экономичный LoRa-only режим.

## 32. Извлечение ID отправителя из LoRa пакетов и использование в alarm_time

### Описание изменений
- Добавлен макрос `POST_SEND_SENDER_ID_AS_ALARM_TIME 0` в `lib/lora_config.hpp` для включения передачи ID отправителя в поле alarm_time POST запросов.
- В `WiFiManager::doHttpPost()` добавлено логирование `alarm_value` при подготовке POST: `ESP_LOGI(TAG, "Alarm time: %d", alarm_value);`.
- В `Control::loRaDataTask()` реализована логика извлечения sender NodeID из заголовков пакетов Meshtastic формата:
  - При пакетах >=16 байт: извлекается sender ID (байты 4-7, little-endian 32-bit) и устанавливается как `last_sender_id`.
  - При пакетах <16 байт (неправильный заголовок): `last_sender_id = 0`, что дает `alarm_time = 00`.
- Логирование извлечения: `"Extracted sender NodeID: <ID>"` или `"Packet too short for mesh header, setting alarm_time to 00"`.

### Технические детали
- **Формат пакетов Meshtastic:** 16-байтный заголовок с sender NodeID в байтах 4-7 (little-endian).
- **Логика:** Когда `POST_SEND_SENDER_ID_AS_ALARM_TIME = 1`, `alarm_time = last_sender_id & 0xFFFF` (последние 16 бит ID LoRa отправителя или 00 при ошибке).
- **Обработка ошибок:** Неправильные заголовки → `alarm_time = 00` вместо случайного значения.

### Обоснование
Позволяет отслеживать отправителя LoRa пакетов через поле `alarm_time` в POST данных. Неправильные пакеты получают `alarm_time = 00` для идентификации проблем.

### Тестирование
Проект компилируется. При получении корректных Meshtastic пакетов `alarm_time` содержит последние 16 бит NodeID отправителя. Неправильные пакеты логгируются с `alarm_time = 00`. В дефолтной конфигурации сохраняется обратная совместимость (`POST_SEND_SENDER_ID_AS_ALARM_TIME = 0`).

## 33. Макрос OLD_LORA_PARS для выбора способа парсинга LoRa пакетов

### Описание изменений
- Добавлен макрос `OLD_LORA_PARS 0` в `lib/lora_config.hpp` для выбора между старым и новым способами обработки LoRa пакетов.
- В `Control::loRaDataTask()` реализована условная логика: при `OLD_LORA_PARS=1` пакеты обрабатываются простым способом с `alarm_time=00`, при `OLD_LORA_PARS=0` используется расширенный парсинг Meshtastic заголовков.
- Модифицирована функция `LoRaCom::getMessage()` для возврата фактической длины принятого пакета через выходной параметр.

### Формат LoRa пакетов и структура данных
**Meshtastic LoRa packet structure (16+ байт заголовок):**
```
0x00-0x03: Destination NodeID (кому отправлено)
0x04-0x07: Sender NodeID (отправитель) ← **КЛЮЧЕВОЕ ПОЛЕ**
0x08-0x0B: Packet ID
0x0C: Flags
0x0D: Channel hash
0x0E: Next-hop
0x0F: Relay node
0x10+: Payload data (protobuf encoded)
```

### Разница в способах парсинга

#### Старый способ парсинга (OLD_LORA_PARS = 1):
```cpp
// Простая обработка в loRaDataTask()
if (m_LoRaCom->getMessage(buffer, sizeof(buffer))) {
    int len = strlen(buffer);  // ❌ ПРОБЛЕМА: strlen() останавливается на первом '\0'
    m_wifiManager->setLastSenderId(0);  // Всегда alarm_time = 0
    ESP_LOGI(TAG, "OLD_LORA_PARS=1: alarm_time=00");
}
```

**Недостатки старого способа:**
- **Проблема с бинарными данными**: `strlen()` считает длину по null-терминатору, но LoRa пакеты могут содержать байты 0x00 в середине
- **Пример ошибки**: Пакет `[01 02 00 03 04]` будет обработан как `[01 02]` (длина 2 вместо 5)
- **Потеря данных**: Заголовки Meshtastic или любые бинарные структуры с null байтами будут усечены
- **Неправильное логирование**: Hex-дампы покажут неполные пакеты

#### Новый способ парсинга (OLD_LORA_PARS = 0):
```cpp
// Расширенная обработка в loRaDataTask()
int receivedLen = 0;
if (m_LoRaCom->getMessage(buffer, sizeof(buffer), &receivedLen)) {
    // ✅ Используем фактическую длину receivedLen
    
    if (receivedLen >= 0x10) {  // Meshtastic заголовок присутствует
        // Извлечение sender NodeID из байтов 4-7 (little-endian)
        uint32_t senderId = packetData[4] | (packetData[5] << 8) | 
                           (packetData[6] << 16) | (packetData[7] << 24);
        m_wifiManager->setLastSenderId(senderId & 0xFFFF);  // 16 бит для alarm_time
        ESP_LOGI(TAG, "Extracted sender NodeID: %lu", (unsigned long)senderId);
    }
    
    // Детектирование типа содержимого (текст/бинарные)
    bool isTextMessage = true;
    for (int i = 0; i < receivedLen && i < 50; i++) {
        if (buffer[i] < 32 && buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != '\t') {
            isTextMessage = false;
            break;
        }
    }
    
    // Обработка в зависимости от типа данных
}
```

**Преимущества нового способа:**
- **Корректная длина пакета**: Фактическая длина от RadioLib, не зависит от null байтов
- **Извлечение метаданных**: Sender NodeID, заголовки Meshtastic
- **Гибкая обработка**: Различное поведение для текстовых и бинарных пакетов
- **Отладка**: Hex-дампы для бинарных данных, текстовая интерпретация для команд

### Возможные ошибки при работе с бинарными данными

#### Проблема 1: Null байты в середине пакета
```cpp
// Пакет: [0xAA, 0xBB, 0x00, 0xCC, 0xDD] (длина 5 байт)

char buffer[256];
strcpy(buffer, packet);  // buffer содержит все байты

// Старый способ:
int wrong_len = strlen(buffer);  // = 2, а не 5! ❌

// Новый способ:
int actual_len = receivedLen;   // = 5, правильная длина ✅
```

#### Проблема 2: Неправильная интерпретация как текст
```cpp
// Пакет с бинарными заголовками выглядит как "мусор" в терминале
ESP_LOGI(TAG, "Received: %s", buffer);  // Выводит мусорные символы
// Решение: hex-dump или проверка isTextMessage
```

#### Проблема 3: Переполнение буфера
```cpp
// Защита от переполнения в новом коде:
buffer[receivedLen] = '\0';  // Null-terminate после фактической длины
```

### Условности выбора режима

- **`OLD_LORA_PARS = 1`**: Для отладки или когда пакеты гарантированно текстовые без null байтов
- **`OLD_LORA_PARS = 0`**: Для полноценной работы с Meshtastic пакетами и бинарными данными

### Технические детали
- Длина пакета определяется `RadioLib` через `readData()` с возвратом количества байтов
- Little-endian конверсия для NodeID: `(byte[4]) | (byte[5]<<8) | (byte[6]<<16) | (byte[7]<<24)`
- Макрос дает runtime контроль без перекомпиляции всего проекта

### Тестирование
Проект компилируется. В логах видно различия: старый режим дает постоянное `alarm_time=00`, новый - извлекает реальные NodeID из Meshtastic заголовков. Режимы переключаются define'ом без изменения кода.

## 34. Автоматическая отправка длины LoRa пакета при новом парсинге

### Описание изменений
- Модифицирована логика в `WiFiManager::doHttpPost()` для автоматической отправки длины LoRa пакета в POST поле `cold` при использовании нового режима парсинга
- Добавлено условие: `if (post_on_lora_mm && (COLD_AS_LORA_PAYLOAD_LEN || !OLD_LORA_PARS))`

### Преимущества новой логики

#### Раньше (после изменения #33):
```cpp
if (post_on_lora_mm && COLD_AS_LORA_PAYLOAD_LEN) {
    cold_value = getLastLoRaPacketLen();
    ESP_LOGI(TAG, "Using LoRa payload length as cold: %ld", cold_value);
} else {
    cold_value = cold_counter++;
    ESP_LOGI(TAG, "Using cold counter: %ld", cold_value);
}
```

#### Теперь (с автоматической отправкой длины):
```cpp
if (post_on_lora_mm && (COLD_AS_LORA_PAYLOAD_LEN || !OLD_LORA_PARS)) {
    cold_value = getLastLoRaPacketLen();
    if (!OLD_LORA_PARS) {
        ESP_LOGI(TAG, "NEW_LORA_PARS: Using LoRa payload length as cold: %ld", cold_value);
    } else {
        ESP_LOGI(TAG, "Using LoRa payload length as cold: %ld", cold_value);
    }
} else {
    cold_value = cold_counter++;
    ESP_LOGI(TAG, "Using cold counter: %ld", cold_value);
}
```

### Разница при выборе режимов

- **`OLD_LORA_PARS = 1`**: Длина пакета отправляется только если `COLD_AS_LORA_PAYLOAD_LEN = 1` (backward compatibility)
- **`OLD_LORA_PARS = 0`**: Длина пакета отправляется автоматически в POST поле `cold` независимо от `COLD_AS_LORA_PAYLOAD_LEN`

### Техническая реализация
- Условие `!OLD_LORA_PARS` добавляется к `COLD_AS_LORA_PAYLOAD_LEN` через логическое ИЛИ
- При новом режиме автоматически используется `getLastLoRaPacketLen()` для поля `cold`
- Сохраняется совместимость со старыми настройками через `COLD_AS_LORA_PAYLOAD_LEN`

### Сценарии использования
1. **Новый режим парсинга** (`OLD_LORA_PARS = 0`): автоматически отправляет длину пакета
2. **Старый режим парсинга** (`OLD_LORA_PARS = 1`): работает как раньше, через `COLD_AS_LORA_PAYLOAD_LEN`

### Тестирование
Проект компилируется и работает корректно. При новом режиме парсинга длинны LoRa пакетов автоматически попадают в поле `cold` POST запросов с соответствующим логированием.

## 35. Парсинг sender_id из заголовков LoRa пакетов и новый API метод getLastSenderId()

### Описание изменений
- Добавлен макрос `PARSE_SENDER_ID_FROM_LORA_PACKETS 0` в `lib/lora_config.hpp` для включения парсинга ID отправителя из заголовков LoRa пакетов
- Добавлен новый публичный метод `getLastSenderId()` в класс `LoRaCom` для получения последнего распознанного sender ID
- Реализована логика парсинга в `LoRaCom::getMessage()`:
  - При `PARSE_SENDER_ID_FROM_LORA_PACKETS = 1`: Получает длину пакета через `getPacketLength()`, разбирает заголовок Meshtastic формата
  - Извлекает sender NodeID из байтов 4-7 (little-endian 32-bit)
  - При длине пакета < 7 байт: sender_id = 1 (короткий пакет)
  - При длине >= 8 байт: sender_id = значение из заголовка
- При `PARSE_SENDER_ID_FROM_LORA_PACKETS = 0`: Сохраняется оригинальная логика без вызова `getPacketLength()`

### Преимущества новой функциональности

#### Новый метод getLastSenderId():
```cpp
// Пример использования
uint32_t senderId = loRaCom.getLastSenderId();
if (senderId == 1) {
    // Обработка коротких пакетов (< 7 байт)
} else if (senderId > 1) {
    // Обработка пакетов с Meshtastic заголовком
    ESP_LOGI(TAG, "Received packet from NodeID: %u", senderId);
}
```

#### Условная логика парсинга:
- **Включен макрос** (`PARSE_SENDER_ID_FROM_LORA_PACKETS = 1`): 
  - Полное извлечение sender ID из заголовков
  - payload включает весь пакет с заголовком
  - длина пакета рассчитывается как `packetLength`
- **Отключен макрос** (`PARSE_SENDER_ID_FROM_LORA_PACKETS = 0`):
  - Никаких вызовов `getPacketLength()` для производительности
  - Оригинальная логика определения длины по null-терминатору
  - Полная backward compatibility

### Технические детали
- **Формат Meshtastic заголовка:** sender NodeID хранится в байтах 4-7 как little-endian 32-bit integer
- **Извлечение ID:** `lastSenderId = (tempBuffer[4]) | (tempBuffer[5] << 8) | (tempBuffer[6] << 16) | (tempBuffer[7] << 24)`
- **Короткие пакеты:** Если длина < 7 байт, не может содержать полный заголовок → sender_id = 1
- **Логирование:** "Parsed sender_id: [ID] from packet length [LEN]" при успешном парсинге

### Сценарии использования

1. **Мониторинг сети:** Определение отправителя каждого полученного пакета
2. **Фильтрация пакетов:** Обработка только от определенных NodeID
3. **Отладка маршрутизации:** Проверка правильности прошивки mesh сети
4. **Статистика:** Подсчет пакетов от каждого узла

### Тестирование
Проект компилируется. При включенном макросе `getLastSenderId()` возвращает корректный sender ID из заголовков Meshtastic пакетов. При отключенном макросе - метод возвращает 0 (не используется). Обеспечена полная обратная совместимость.

## 36. Поддержка альтернативного Flask сервера для HTTP POST запросов

### Описание изменений
- Добавлен макрос `USE_FLASK_SERVER 0` в `lib/lora_config.hpp` для выбора между PHP и Flask серверами
- Добавлены параметры Flask сервера в `lib/fake_network_definitions.h`:
  - `DEFAULT_FLASK_SERVER_IP = "xx.xx.xx.xx"` (IP адрес Flask сервера)
  - `DEFAULT_FLASK_SERVER_PORT = "xx"` (порт Flask сервера)
  - `DEFAULT_FLASK_SERVER_PATH = "/api/lora"` (путь API)
  - Остальные параметры: user_id="new_device_001", user_location="location_after_clear"

### Различия между PHP и Flask серверами

#### PHP сервер (USE_FLASK_SERVER = 0):
- **Формат данных:** application/x-www-form-urlencoded
- **Пример POST данных:** `api_key=abc123&user_id=test&cold=100&hot=50&alarm_time=12345`
- **Порт:** 80 (стандартный HTTP)
- **Параметры:** api_key, user_id, user_location, cold, hot, alarm_time

#### Flask сервер (USE_FLASK_SERVER = 1):
- **Формат данных:** application/json
- **Пример POST данных:** `{"user_id":"new_device_001","user_location":"location_after_clear","sender_nodeid":"A1B2C3D4","destination_nodeid":"FFFFFFFF","full_packet_len":45,"signal_level_dbm":-85}`
- **Порт:** 5001 (конфигурируемый)
- **URL:** http://84.252.143.212:5001/api/lora
- **CURL пример:** `curl -X POST http://84.252.143.212:5001/api/lora -H "Content-Type: application/json" -d '{"user_id":"new_device_001","user_location":"location_after_clear","sender_nodeid":"A1B2C3D4","destination_nodeid":"FFFFFFFF","full_packet_len":45,"signal_level_dbm":-85}'`

### Детальные поля для Flask сервера

| Поле JSON | Тип | Описание | Пример значения |
|-----------|-----|----------|----------------|
| `user_id` | string | ID устройства (строка) | `"new_device_001"` |
| `user_location` | string | Локация устройства (строка) | `"location_after_clear"` |
| `sender_nodeid` | string | Sender NodeID как HEX строка (8 символов) | `"A1B2C3D4"` |
| `destination_nodeid` | string | Destination NodeID как HEX строка (8 символов) | `"FFFFFFFF"` (broadcast) |
| `signal_level_dbm` | Уровень сигнала RSSI в dBm | `-85` |

### Технические детали реализации
- В `WiFiManager::loadSettings()` условная загрузка параметров в зависимости от макроса
- В `WiFiManager::doHttpPost()` условная генерация POST данных:
  - Для Flask: JSON объект с полями
  - Для PHP: form-encoded строка
- Автоматический выбор Content-Type заголовка
- Обновленное логирование с корректными CURL примерами для каждого типа сервера
- Ping функциональность использует configurable порт

### Сценарии использования

1. **Тестирование на PHP сервере** (USE_FLASK_SERVER = 0):
   - Разработка и отладка
   - Backward compatibility с существующими серверами

2. **Продакшн на Flask сервере** (USE_FLASK_SERVER = 1):
   - Новые развертывания с JSON API
   - Современная архитектура сервера
   - Поддержка структурированных данных

### Тестирование
Проект компилируется. При `USE_FLASK_SERVER = 1` устройство отправляет JSON данные на Flask сервер. При `USE_FLASK_SERVER = 0` - использует традиционный PHP формат. Логи отображают корректные CURL примеры для каждого режима.
