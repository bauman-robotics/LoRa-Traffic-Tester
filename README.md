# Mini LoRa Transceiver

Проект для тестирования нагрузки  LoRa сети на базе ESP32-C3 Supermini и радиомодема E22-900M22S (SX1262).

Монитор загрузки сети без MQTT. 
Три приемных LoRa модуля с разными антеннами, слушают Long Fast, 
при обнаружении пакетов, отпраляют на сервер 
- уровень сигнала, 
- длину пакета, 
- заголовок пакета (ID ноды назначения).  
Ноды могут находиться в любой локации, необходимо обеспечить подключение к интернету через WiFi. 
Онлайн визуальная статистика доступна с любого устройства через браузер. 
Количество нод ограничено количеством одновременных запросов к серверной базе данных (100). 


## Описание

![Grafana Logs](pics/06_Traffic_monitor_Pro_3_signals.png)
![Gui](pics/07_LoRa_Mon_Gui.png)
![Gui upd](pics/08_Grafana_Pro_dest_send_view.png)


Этот проект реализует LoRa трансивер для беспроводной связи на базе ESP32-C3 и модуля SX1262. Поддерживает передачу и прием данных через Serial интерфейс, управление через команды, сохранение данных во flash память.

Проект основан на [mini_lora_transceiver](https://github.com/lukuky64/mini_lora_transceiver/) с добавлением графического интерфейса для управления.

## Структура проекта

```
mini_lora_transceiver/
├── src/
│   └── main.cpp                 # Основной файл программы
├── lib/
│   ├── lora_config.hpp          # Конфигурация LoRa и GPIO пинов
│   ├── commander/               # Модуль команд
│   │   ├── commander.hpp
│   │   └── commander.cpp
│   ├── control/                 # Основной контроллер
│   │   ├── control.hpp
│   │   └── control.cpp
│   ├── LoRaCom/                 # LoRa коммуникации
│   │   ├── LoRaCom.hpp
│   │   └── LoRaCom.cpp
│   ├── SerialCom/               # Serial коммуникации
│   │   ├── SerialCom.hpp
│   │   └── SerialCom.cpp
│   └── fileSystem/              # Работа с flash памятью
│       ├── saveFlash.hpp
│       └── saveFlash.cpp
├── include/
│   └── README                   # Заголовочные файлы
├── test/
│   └── README                   # Тесты
├── bin/                         # Бинарные файлы (создается при сборке)
├── config.json                  # Конфигурационный файл
├── platformio.ini               # Конфигурация PlatformIO
├── build_script.py              # Скрипт сборки
├── upload_script.py             # Скрипт загрузки
├── clean_script.py              # Скрипт очистки
├── gui.py                       # GUI для управления
├── requirements.txt             # Зависимости Python
└── README.md                    # Этот файл
```

## Конфигурация GPIO

GPIO пины определены в `lib/lora_config.hpp`:

### Текущая конфигурация:
- **BUTTON_PIN**: GPIO9 (BOOT button)
- **LORA_RESET**: GPIO5 (RST от SX1262)
- **LORA_DIO1**: GPIO3 (DIO1 от SX1262)
- **LORA_RXEN**: GPIO2 (RX enable)
- **LORA_BUSY**: GPIO4 (BUSY от SX1262)
- **LORA_SCK**: GPIO10 (SPI clock)
- **LORA_MISO**: GPIO6 (SPI MISO)
- **LORA_MOSI**: GPIO7 (SPI MOSI)
- **LORA_CS**: GPIO8 (SPI CS)
- **INDICATOR_LED1**: GPIO2 (LED индикатор)

## Установка и настройка

### Требования
- Python 3.8+
- PlatformIO CLI
- Виртуальное окружение Python (venv)

### Настройка
1. Клонируйте репозиторий
2. Создайте виртуальное окружение в текущей папке: `python3 -m venv venv`
3. Активируйте виртуальное окружение: `source venv/bin/activate`
4. Установите зависимости: `pip install -r requirements.txt`
5. Настройте GPIO пины в `lib/lora_config.hpp` (если необходимо)
6. Настройте пути в `config.json` под вашу систему

### Конфигурационный файл config.json
```json
{
  "venv_activate": "venv/bin/activate",
  "wifi_enabled": true,
  "post_mode": "time",
  "status_enabled": false,
  "status_interval": "30",
  "log_short": true,
  "gain": "20",
  "freq": "869.075",
  "spreading_factor": "11",
  "bandwidth": "250"
}

## Сборка и загрузка

### Сборка
```bash
python3 build_script.py
```
Собирает проект и копирует бинарные файлы в папку `bin/`.

### Загрузка
```bash
# Только загрузка существующего бинарника
python3 upload_script.py

# Сборка + загрузка
python3 upload_script.py --build
```

### Очистка
```bash
python3 clean_script.py
```
Удаляет папки `.pio`, `bin`.

## Использование

### Serial команды
Подключитесь к ESP32 через Serial (115200 baud) и используйте команды:

- `help` - список команд
- `command update gain <value>` - установить усиление
- `command update freqMhz <value>` - установить частоту
- `command update sf <value>` - установить spreading factor
- `command update bwKHz <value>` - установить bandwidth
- `command set status <0|1>` - включить/выключить автоматическую отправку статуса
- `data <payload>` - отправить данные
- `flash` - прочитать сохраненные данные из flash

### GUI
Запустите графический интерфейс:
```bash
python3 gui.py
```

## Особенности

- Автоматическая активация venv среды
- Сохранение данных во flash память
- Поддержка команд через Serial
- LED индикация состояния
- Настраиваемые параметры LoRa

## Разработка

Проект использует:
- **ESP32-C3** микроконтроллер
- **SX1262** LoRa модуль
- **RadioLib** библиотека для LoRa
- **PlatformIO** для сборки
- **ESP-IDF** framework
