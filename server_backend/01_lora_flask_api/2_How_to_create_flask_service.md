# Настройка Flask API сервиса для LoRa

## Краткий план настройки

Для запуска LoRa Flask API сервиса выполните следующие шаги:

1. **Создать виртуальное окружение** Python
2. **Установить зависимости** из requirements.txt
3. **Настроить конфигурацию** базы данных и сервера
4. **Протестировать запуск** в development режиме
5. **Настроить production сервер** с Gunicorn
6. **Создать systemd сервис** для автозапуска
7. **Протестировать API endpoints**
8. **Настроить мониторинг** и логи

> **ВНИМАНИЕ:** В конфигурационных файлах используются переменные окружения. Установите их значения:
> - `DB_HOST` → хост базы данных PostgreSQL
> - `DB_NAME` → название базы данных
> - `DB_USER` → имя пользователя базы данных
> - `DB_PASSWORD` → пароль пользователя базы данных
> - `DB_PORT` → порт PostgreSQL (обычно 5432)
> - `FLASK_HOST` → хост для Flask сервера (0.0.0.0 для всех интерфейсов)
> - `FLASK_PORT` → порт Flask сервера (5001)
> - `FLASK_DEBUG` → режим отладки (False для production)

## Создание виртуального окружения

### Зачем нужно виртуальное окружение?

- **Изоляция зависимостей** проекта от системных пакетов
- **Предотвращение конфликтов** версий библиотек
- **Упрощение деплоя** и управления зависимостями
- **Чистота системы** - легкое удаление проекта

### Создание и активация venv

```bash
# Переход в директорию проекта
cd /root/lora_flask_api

# Создание виртуального окружения
python3 -m venv venv

# Активация venv
source venv/bin/activate

# Проверка - в приглашении командной строки появится (venv)
(venv) root@host:~/lora_flask_api#
```

### Деактивация venv
```bash
deactivate
```

## Установка зависимостей

### Файл requirements.txt
```
Flask>=2.3.0
psycopg2-binary>=2.9.0
python-dotenv>=1.0.0
gunicorn>=21.0.0
```

### Установка пакетов
```bash
# Активируйте venv если еще не активирован
source venv/bin/activate

# Установка всех зависимостей
pip install -r requirements.txt

# Проверка установленных пакетов
pip list
```

### Проверка установки
```bash
python -c "import flask, gunicorn, psycopg2; print('Все пакеты установлены успешно')"
```

## Структура проекта

```
/root/lora_flask_api/
├── venv/                 # Виртуальное окружение
├── lora_api.py          # Основной файл приложения Flask
├── config.py            # Конфигурация (БД, хост, порт)
├── requirements.txt     # Зависимости проекта
└── .env                 # Переменные окружения (опционально)
```

### Пример config.py
```python
import os

# Конфигурация базы данных PostgreSQL
DB_CONFIG = {
    'host': os.getenv('DB_HOST', 'localhost'),
    'database': os.getenv('DB_NAME', 'lora_db'),
    'user': os.getenv('DB_USER', 'lora_user'),
    'password': os.getenv('DB_PASSWORD', 'password'),
    'port': os.getenv('DB_PORT', 5432)
}

# Настройки сервера
DEBUG = os.getenv('FLASK_DEBUG', False)
HOST = os.getenv('FLASK_HOST', '0.0.0.0')
PORT = int(os.getenv('FLASK_PORT', 5001))
```

## Запуск в development режиме

### Прямой запуск через Python
```bash
cd /root/lora_flask_api
source venv/bin/activate
python lora_api.py
```

### Ожидаемый вывод
```
Starting LoRa API server...
Database: lora_db@localhost:5432
Server: http://0.0.0.0:5001

API endpoints:
  GET    /api/lora          - Get all data
  GET    /api/lora/<id>     - Get single record
  POST   /api/lora          - Add new record
  GET    /api/health        - Health check
  GET    /api/lora/view     - HTML view
  GET/POST/DELETE /api/lora/clear - Clear table
 * Serving Flask app 'lora_api'
 * Debug mode: off
WARNING: This is a development server. Do not use it in a production deployment.
 * Running on all addresses (0.0.0.0)
 * Running on http://127.0.0.1:5001
```

## Запуск с Gunicorn (Production)

### Что такое Gunicorn?

Production-ready WSGI HTTP сервер для Python приложений:
- Обрабатывает multiple запросы одновременно
- Автоматическое управление worker процессами
- Высокая производительность и надежность

### Базовый запуск
```bash
cd /root/lora_flask_api
source venv/bin/activate
gunicorn --bind 0.0.0.0:5001 --workers 2 lora_api:app
```

### Продвинутая конфигурация
```bash
gunicorn --bind 0.0.0.0:5001 \
         --workers 4 \
         --worker-class sync \
         --timeout 120 \
         --max-requests 1000 \
         --access-logfile - \
         lora_api:app
```

### Параметры Gunicorn

- `--bind` - адрес и порт для прослушивания
- `--workers` - количество worker процессов (рекомендуется: CPU cores * 2 + 1)
- `--worker-class` - тип воркеров (sync, gevent, eventlet)
- `--timeout` - таймаут обработки запроса в секундах
- `--max-requests` - максимальное количество запросов на worker перед перезапуском
- `--access-logfile` - файл логов доступа (- для вывода в stdout)

### Ожидаемый вывод Gunicorn
```
[2025-11-28 15:09:35 +0300] [3408144] [INFO] Starting gunicorn 23.0.0
[2025-11-28 15:09:35 +0300] [3408144] [INFO] Listening at: http://0.0.0.0:5001 (3408144)
[2025-11-28 15:09:35 +0300] [3408144] [INFO] Using worker: sync
[2025-11-28 15:09:35 +0300] [3408145] [INFO] Booting worker with pid: 3408145
[2025-11-28 15:09:35 +0300] [3408146] [INFO] Booting worker with pid: 3408146
```

## Создание systemd сервиса

### Создание файла сервиса
```bash
sudo nano /etc/systemd/system/lora-api.service
```

```
# sudo nano /etc/systemd/system/lora-api.service
[Unit]
Description=LoRa Flask API Server
After=network.target postgresql.service
Wants=postgresql.service

[Service]
Type=simple
User=root
Group=root
WorkingDirectory=/root/lora_flask_api
Environment=PATH=/root/lora_flask_api/venv/bin
Environment=PYTHONUNBUFFERED=1

# Используем Gunicorn вместо прямого запуска Python
ExecStart=/root/lora_flask_api/venv/bin/gunicorn \
          --bind 0.0.0.0:5001 \
          --workers 2 \
          --timeout 60 \
          --access-logfile - \
          --error-logfile - \
          lora_api:app

Restart=always
RestartSec=10

# Логирование
StandardOutput=journal
StandardError=journal

[Install]
WantedBy=multi-user.target

```

### Настройка прав и активация сервиса
```bash
# Даем правильные права файлу
sudo chmod 644 /etc/systemd/system/lora-api.service

# Перезагружаем демон systemd
sudo systemctl daemon-reload

# Включаем автозагрузку при старте системы
sudo systemctl enable lora-api.service

# Запускаем сервис
sudo systemctl start lora-api.service

# Проверяем статус
sudo systemctl status lora-api.service
```

### Управление сервисом
```bash
# Статус сервиса
sudo systemctl status lora-api

# Запуск сервиса
sudo systemctl start lora-api

# Остановка сервиса
sudo systemctl stop lora-api

# Перезапуск сервиса
sudo systemctl restart lora-api

# Просмотр логов
sudo journalctl -u lora-api -f

# Проверка автозагрузки
sudo systemctl is-enabled lora-api
```

## Тестирование API

### Health Check
```bash
# Проверка работы API и подключения к БД
curl http://127.0.0.1:5001/api/health
```

Ожидаемый ответ:
```json
{
  "status": "healthy",
  "database": "connected",
  "timestamp": "2025-11-28T15:20:00.123456"
}
```

### Получение всех записей
```bash
curl http://127.0.0.1:5001/api/lora
```

### Добавление новой записи
```bash

curl -X POST http://127.0.0.1:5001/api/lora -H "Content-Type: application/json" -d '{"user_id": "new_device_001", "user_location": "location_after_clear", "cold": 100, "hot": 50}'
```

### Очистка таблицы
```bash
# Через GET с подтверждением
curl "http://127.0.0.1:5001/api/lora/clear?confirm=yes"

# Через POST с подтверждением
curl -X POST http://127.0.0.1:5001/api/lora/clear \
  -H "Content-Type: application/json" \
  -d '{"confirmation": "YES_CLEAR_ALL"}'
```

## Мониторинг и логи

### Просмотр логов systemd сервиса
```bash
# Логи в реальном времени
sudo journalctl -u lora-api -f

# Последние 100 записей
sudo journalctl -u lora-api -n 100

# Логи за определенную дату
sudo journalctl -u lora-api --since "2025-11-28 00:00:00" --until "2025-11-28 23:59:59"
```

### Проверка процессов
```bash
# Проверка работающих процессов Gunicorn
ps aux | grep gunicorn

# Проверка открытых портов
netstat -tlnp | grep 5001
# или
ss -tlnp | grep 5001
```

### Мониторинг ресурсов
```bash
# Потребление памяти и CPU
top -p $(pgrep gunicorn | head -1 | tail -1)

# Дисковое пространство
df -h /root/lora_flask_api
```

## Устранение неполадок

### Сервис не запускается
```bash
# Подробные логи
sudo journalctl -u lora-api --no-pager

# Проверка синтаксиса Python файла
python -m py_compile lora_api.py

# Проверка доступности порта
telnet 127.0.0.1 5001
```

### Проблемы с базой данных
```bash
# Проверка подключения к PostgreSQL
psql -h localhost -U lora_user -d lora_db

# Проверка существования таблицы
psql -h localhost -U lora_user -d lora_db -c "\dt"
```

### Проблемы с зависимостями
```bash
# Активируйте venv
source venv/bin/activate

# Переустановите зависимости
pip install --force-reinstall -r requirements.txt

# Проверьте версии
pip list | grep -E "(Flask|gunicorn|psycopg2)"
```

## Важные примечания

- **Всегда используйте виртуальное окружение** для изоляции зависимостей
- **Для production всегда используйте Gunicorn** или другой WSGI сервер
- **Не используйте Flask development сервер** в production
- **Регулярно обновляйте зависимости** для безопасности
- **Настройте бэкапы базы данных**
- **Мониторьте логи и производительность**

Это руководство покрывает полный цикл установки, настройки и запуска LoRa Flask API в production-окружении.
