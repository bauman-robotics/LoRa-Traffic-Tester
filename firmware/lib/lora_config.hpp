#pragma once

// DIO2 подключен к антенному переключателю, и вам не нужно его подключать к MCU.
#define SX126X_DIO2_AS_RF_SWITCH
// DIO3 для TCXO 1.8V
#define SX126X_DIO3_TCXO_VOLTAGE (1.8)

#define LORA_RESET (5)
#define LORA_DIO1  (3)
#define LORA_BUSY  (4)

#define LORA_SCK  (10)
#define LORA_MISO (6)
#define LORA_MOSI (7)
#define LORA_CS   (8)

// Настройки LoRa по умолчанию (совместимы с модулем E80)
#define LORA_FREQUENCY 869.075f
#define LORA_POWER 20

// Режим симуляции: если 1, пропустить инициализацию аппаратного LoRa и использовать фейковый режим
#define FAKE_LORA 0

// Интервал отправки статусов (можно изменить через команды)
extern unsigned long status_Interval;

// Конфигурация WiFi
#define WIFI_ENABLE 1  // Включить функционал WiFi (управление во время выполнения через GUI)
#define WIFI_USE_CUSTOM_MAC 0  // Использовать кастомный случайный MAC адрес для WiFi (0=использовать ESP по умолчанию, 1=генерировать случайный)
#define WIFI_USE_FIXED_MAC 0  // Если 1, использовать фиксированный MAC адрес указанный ниже
//#define WIFI_FIXED_MAC_ADDRESS {0x1c, 0xdb, 0xd4, 0xc6, 0x77, 0xf0}  // Байты фиксированного MAC адреса (используется если WIFI_USE_FIXED_MAC=1)
#define WIFI_FIXED_MAC_ADDRESS {0x1c, 0xdb, 0xd4, 0xC3, 0xC9, 0xD4}
#define WIFI_CONNECT_ATTEMPTS 5  // Количество попыток подключения к WiFi
#define WIFI_CONNECT_INTERVAL_MS 5000  // Интервал между попытками подключения
//#define WIFI_CONNECT_INTERVAL_MS 10000  // Тестовый интервал между попытками подключения
#define WIFI_DEBUG_FIXES 1  // Включить исправления отладки WiFi и сканирование (установить 1 для ESP32-C3 supermini с проблемами)
#define WIFI_AUTO_TX_POWER_TEST 0    // Включить автоматическое тестирование разных уровней мощности TX (0=использовать WIFI_TX_POWER_VARIANT, 1=авто тест)
#define WIFI_ATTEMPTS_PER_VARIANT 2  // Количество попыток на вариант мощности TX в авто тесте

// Настройки сети включены из lib/network_definitions.h или резервные
// Редактируйте lib/network_definitions.h чтобы изменить настройки во время компиляции
// Если основного файла нет, редактируйте lib/fake_network_definitions.h вместо этого

#define USE_SYSTEM_NETWORK

#ifdef USE_SYSTEM_NETWORK
#include "../../../network_definitions.h"
#else
#include "fake_network_definitions.h"  // Резервные значения по умолчанию
#endif

#define USE_HTTPS 1  // Если 1, использовать HTTPS для связи с сервером; если 0, использовать HTTP
#define USE_INSECURE_HTTPS 1  // Если 1, пропустить проверку SSL сертификатов (эквивалент curl -k); если 0, проверять сертификаты
#define POST_BATCH_ENABLED 1  // Включить отправку нескольких LoRa пакетов в одном POST запросе (пакетный режим)


#define POST_INTERVAL_EN 0  // Включить периодические POST запросы (можно управлять через GUI)
#define POST_EN_WHEN_LORA_RECEIVED 1  // Отправлять POST только при получении LoRa пакета
#define POST_HOT_AS_RSSI 1  // Если 1, использовать RSSI как hot параметр при POST triggered LoRa receive; если 0, использовать счетчик успешных POST
#define POST_SEND_SENDER_ID_AS_ALARM_TIME 1  // Если 1, использовать sender ID как alarm time вместо ALARM_TIME + random
#define OLD_LORA_PARS 0  // Если 1, использовать старый парсинг пакетов (alarm_time=00), если 0, использовать новый Meshtastic header парсинг  === 1 ========================================
#define PARSE_SENDER_ID_FROM_LORA_PACKETS 1  // Если 1, парсить sender_id из LoRa packet header когда длина пакета > 7 байт, использовать 1 иначе === 2 ========================================
#define USE_FLASK_SERVER 1  // Если 1, использовать Flask сервер (84.252.143.212:5001); если 0, использовать PHP сервер (по умолчанию)

#define SERVER_PING_ENABLED 0  // Включить периодический ping сервера
#define POST_INTERVAL_MS 10000  // Интервал между POST запросами в мс (если включено)
#define PING_INTERVAL_MS 5000  // Интервал между ping в мс
#define SERVER_CONNECTION_TIMEOUT_MS 5000  // Таймаут на установление соединения с сервером
#define WIFI_POST_DELAY_MS 10000  // Задержка после подключения WiFi перед отправкой initial POST
#define POST_RESPONSE_INITIAL_DELAY_MS 1000  // Начальная задержка перед ожиданием ответа сервера
#define POST_RESPONSE_TOTAL_TIMEOUT_MS 8000  // Общий таймаут ожидания ответа сервера
#define HOT_WATER 0  // Значение по умолчанию для поля hot water
#define ALARM_TIME 200  // Поле alarm time

// Начальное значение счетчика cold water
#define COLD_INITIAL 1

// Настройки периодической отправки LoRa статусов
#define LORA_STATUS_ENABLED 0  // Включить/отключить периодические LoRa статус пакеты
#define LORA_STATUS_INTERVAL_SEC 10  // Интервал по умолчанию для LoRa статус пакетов в секундах
#define LORA_STATUS_SHORT_PACKETS 1  // 0=полный пакет, 1=короткий 2-байтовый счетчик

// Совместимость с Mesh
#define MESH_COMPATIBLE 1  // Если 1, установить BW/SF/CR/sync для соответствия Meshtastic SHORT_FAST при приеме их пакетов
#define MESH_SYNC_WORD 0x2B  // Приватное Meshtastic sync слово
#define MESH_FREQUENCY 869.075f  // Частота для использования Mesh совместимости
#define MESH_BANDWIDTH 250  // Полоса для Mesh (оригинально узкая)
#define MESH_SPREADING_FACTOR 11  // SF для Mesh (сопоставлено с Meshtastic для trace пакетов)
#define MESH_CODING_RATE 5  // CR для Mesh

// Выбор режима приема
#define DUTY_CYCLE_RECEPTION 1  // Если 1, использовать Meshtastic-style duty cycle reception (энергоэффективный); если 0, использовать continuous receive (для тестирования трафика)


// Если 1, отправлять длину LoRa packet payload как cold value в POST запросе (когда POST_EN_WHEN_LORA_RECEIVED=1)
#define COLD_AS_LORA_PAYLOAD_LEN 1

// Счетчик для cold water (будет инкрементироваться)
extern unsigned long cold_counter;

#define WIFI_TX_POWER_VARIANT 7  // Variant for TX power testing (0-9 for different levels)
#if WIFI_TX_POWER_VARIANT == 0
#define WIFI_TX_POWER WIFI_POWER_20dBm  // 20 dBm (максимум для ESP32)
#elif WIFI_TX_POWER_VARIANT == 1
#define WIFI_TX_POWER WIFI_POWER_19_5dBm  // 19.5 dBm
#elif WIFI_TX_POWER_VARIANT == 2
#define WIFI_TX_POWER WIFI_POWER_19dBm  // 19 dBm
#elif WIFI_TX_POWER_VARIANT == 3
#define WIFI_TX_POWER WIFI_POWER_18_5dBm  // 18.5 dBm
#elif WIFI_TX_POWER_VARIANT == 4
#define WIFI_TX_POWER WIFI_POWER_18dBm  // 18 dBm
#elif WIFI_TX_POWER_VARIANT == 5
#define WIFI_TX_POWER WIFI_POWER_15dBm  // 15 dBm
#elif WIFI_TX_POWER_VARIANT == 6
#define WIFI_TX_POWER WIFI_POWER_10dBm  // 10 dBm
#elif WIFI_TX_POWER_VARIANT == 7
#define WIFI_TX_POWER WIFI_POWER_8_5dBm  // 8.5 dBm (низкая мощность для тестирования, сейчас используется)
#elif WIFI_TX_POWER_VARIANT == 8
#define WIFI_TX_POWER WIFI_POWER_2dBm  // 2 dBm (минимум)
#else
#define WIFI_TX_POWER WIFI_POWER_8_5dBm  // Default to 8.5dBm
#endif


//===  Не поднимался вайфай на esp32c3supermini. Что сработало ===
// Перенес WiFi.setTxPower((wifi_power_t)WIFI_TX_POWER) ПЕРЕД WiFi.begin() для исправления
// AUTH_EXPIRE на ESP32-C3 Super Mini. Также добавил логи "WiFi TX power set before begin" для диагностики.

//=== Конфигурации для разных типов устройств ===

// Установите DEVICE_TYPE:
// 0 = RECEIVER - устройство для приема и передачи данных по WiFi
// 1 = TRANSMITTER - устройство только для передачи LoRa статусов
// 2 = TEST_POST_TRANSMITTER - Передача тестовых пакетов на сервер через заданный промежуток времени. 

#define DEVICE_TYPE_LORA_RECEIVER 0
#define DEVICE_TYPE_LORA_TRANSMITTER 1
#define DEVICE_TYPE_TEST_HTTP_POST 2

#define DEVICE_TYPE 0  // 0=приемник, 1=передатчик

#if DEVICE_TYPE == 0  // RECEIVER - устройство для приема данных и передачи по WiFi
    #define WIFI_ENABLE 1
    #define POST_EN_WHEN_LORA_RECEIVED 1
    #define LORA_STATUS_ENABLED 0
    #define POST_INTERVAL_EN 0
    #define FAKE_LORA 0

#elif DEVICE_TYPE == 1  // TRANSMITTER - устройство для передачи LoRa статусов
    #define WIFI_ENABLE 0
    #define POST_EN_WHEN_LORA_RECEIVED 0
    #define LORA_STATUS_ENABLED 1
    // ОтключаемPOST режимы для экономии
    #define POST_INTERVAL_EN 0
    #define WIFI_DEBUG_FIXES 0
    #define WIFI_AUTO_TX_POWER_TEST 0

#elif DEVICE_TYPE == 2  // TRANSMITTER - устройство для передачи test пакетов
    #define WIFI_ENABLE 1
    #define POST_EN_WHEN_LORA_RECEIVED 0
    #define LORA_STATUS_ENABLED 0
    #define POST_INTERVAL_EN 1

#else
    #error "Invalid DEVICE_TYPE. Must be 0 (RECEIVER) or 1 (TRANSMITTER)"
#endif
