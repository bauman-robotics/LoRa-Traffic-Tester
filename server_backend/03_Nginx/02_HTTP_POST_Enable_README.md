# Разрешение HTTP POST запросов для ESP32

## Проблема

ESP32 не поддерживает HTTPS без дополнительных библиотек, поэтому для отправки POST-запросов на API нужно разрешить HTTP-доступ к `/api/` endpoints.

## Решение

Модифицировать Nginx конфигурацию, чтобы:
- Оставить HTTPS для Grafana и админ-доступа
- Разрешить HTTP для API endpoints `/api/`
- Поддерживать CORS для ESP32

## Шаг 1: Резервная копия

```bash
sudo cp /etc/nginx/sites-available/lora-api /etc/nginx/sites-available/lora-api.backup
```

## Шаг 2: Модификация конфигурации

Отредактируйте файл:

```bash
sudo nano /etc/nginx/sites-available/lora-api
```

### Изменения в HTTPS сервере (port 443)

Оставьте без изменений - HTTPS для Grafana и защищённых endpoints.

### Изменения в HTTP сервере (port 80)

Замените редирект на прокси для API:

```nginx
# ДО (только редирект)
server {
    listen 80;
    server_name YOUR-IP-WITH-DASHES.nip.io;
    return 301 https://$server_name$request_uri;
}

# ПОСЛЕ (HTTP API + редирект)
server {
    listen 80;
    server_name YOUR-IP-WITH-DASHES.nip.io;

    # API через HTTP (для ESP32)
    location /api/ {
        # Прокси на локальный Flask API
        proxy_pass http://127.0.0.1:5001;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        # CORS заголовки для ESP32
        add_header Access-Control-Allow-Origin * always;
        add_header Access-Control-Allow-Methods "GET, POST, PUT, DELETE, OPTIONS" always;
        add_header Access-Control-Allow-Headers "Content-Type, Authorization" always;

        # Обработка preflight OPTIONS запросов
        if ($request_method = 'OPTIONS') {
            add_header Access-Control-Allow-Origin * always;
            add_header Access-Control-Allow-Methods "GET, POST, PUT, DELETE, OPTIONS" always;
            add_header Access-Control-Allow-Headers "Content-Type, Authorization" always;
            add_header Content-Type 'text/plain charset=UTF-8';
            add_header Content-Length 0;
            return 204;
        }

        # Таймауты для медленных ESP32
        proxy_connect_timeout 60;
        proxy_send_timeout 60;
        proxy_read_timeout 60;
        send_timeout 60;

        # Логирование для отладки
        access_log /var/log/nginx/api_access.log;
        error_log /var/log/nginx/api_error.log;
    }

    # Редирект остальных запросов на HTTPS
    location / {
        return 301 https://$server_name$request_uri;
    }
}
```

## Шаг 3: Проверка конфигурации

```bash
sudo nginx -t
```

Если ошибки:
```bash
sudo nginx -c /etc/nginx/nginx.conf
```

## Шаг 4: Перезагрузка Nginx

```bash
sudo systemctl reload nginx
```

## Шаг 5: Тестирование

### Тест HTTP API (для ESP32)

```bash
# Тест POST через HTTP
curl -v -X POST http://127.0.0.1/api/lora \
  -H "Content-Type: application/json" \
  -d '{
    "user_id": "esp32_test",
    "user_location": "test_location",
    "cold": 123,
    "hot": 456,
    "alarm_time": 789,
    "signal_level_dbm": 50
  }'

# Ожидаемый ответ: HTTP 200 с JSON
```

### Тест HTTPS API

```bash
# Тот же запрос через HTTPS
curl -v -X POST https://127.0.0.1/api/lora \
  -H "Content-Type: application/json" \
  -d '{
    "user_id": "esp32_test_https",
    "user_location": "test_location",
    "cold": 123,
    "hot": 456,
    "alarm_time": 789,
    "signal_level_dbm": 50
  }'
```

### Тест CORS

```bash
# Проверка OPTIONS запроса
curl -v -X OPTIONS http://YOUR-IP/api/lora \
  -H "Origin: http://esp32.local" \
  -H "Access-Control-Request-Method: POST" \
  -H "Access-Control-Request-Headers: Content-Type"
```

### Тест Grafana

```bash
# Должен редиректить на HTTPS
curl -I http://YOUR-IP/grafana/
# Ожидаемый: 301 Moved Permanently
```

## Шаг 6: Логи

### Nginx логи

```bash
# API логи
sudo tail -f /var/log/nginx/api_access.log
sudo tail -f /var/log/nginx/api_error.log

# Основные логи
sudo tail -f /var/log/nginx/access.log
sudo tail -f /var/log/nginx/error.log
```

### Flask API логи

```bash
sudo journalctl -u lora-api -f
```

## Шаг 7: Мониторинг

### Проверка статуса

```bash
# Nginx
sudo systemctl status nginx

# API сервис
sudo systemctl status lora-api

# Grafana
sudo systemctl status grafana-server
```

### Проверка endpoints

```bash
# HTTP API health
curl http://YOUR-IP/api/health

# HTTPS API health
curl https://YOUR-IP/api/health
```

## Расширенная конфигурация

### Rate limiting для API

```nginx
location /api/ {
    # Rate limit: 10 requests per minute per IP
    limit_req zone=api_zone burst=10 nodelay;

    # ... остальные настройки
}
```

В http секции nginx.conf добавить:
```nginx
limit_req_zone $binary_remote_addr zone=api_zone:10m rate=10r/m;
```

### Дополнительные CORS заголовки

```nginx
location /api/ {
    # ... существующие настройки

    add_header Access-Control-Max-Age 86400 always;
    add_header Access-Control-Allow-Credentials false always;
}
```

## Troubleshooting

### 404 Not Found на /api/

Проверить:
```bash
# Статус Flask API
sudo systemctl status lora-api

# Логи Flask
sudo journalctl -u lora-api --since "1 hour ago"

# Тест локального API
curl http://127.0.0.1:5001/api/health
```

### CORS ошибки

В логах браузера или ESP32 проверить заголовки.

### SSL redirect loop

Если бесконечный редирект:
```bash
# Проверить конфигурацию
sudo nginx -T | grep -A 20 "server {"
```

### ESP32 не может подключиться

Проверить:
- Firewall: `sudo ufw status`
- SELinux/AppArmor если включены
- DNS разрешение ESP32

## Безопасность

### Ограничение по IP

Разрешить только локальную сеть:

```nginx
location /api/ {
    allow 192.168.1.0/24;
    allow 10.0.0.0/8;
    deny all;

    # ... остальные настройки
}
```

### Логирование запросов

```nginx
location /api/ {
    access_log /var/log/nginx/esp32_api.log json_combined;
    error_log /var/log/nginx/esp32_api_error.log warn;

    # ... остальные настройки
}
```

## Производительность

### Оптимизация для ESP32

```nginx
location /api/ {
    # Маленькие буферы для ESP32
    proxy_buffer_size 4k;
    proxy_buffers 8 4k;

    # Быстрые таймауты
    proxy_connect_timeout 10;
    proxy_send_timeout 15;
    proxy_read_timeout 20;

    # ... остальные настройки
}
```

## Тестирование с ESP32

### Настройка Test_WiFi

В `wifi_test_config.hpp` или через build_script.py изменить:
```cpp
#define DEFAULT_SERVER_IP "YOUR-IP"
#define DEFAULT_SERVER_PORT "80"  // HTTP вместо 5001
```

### Логи ESP32

В мониторе должны быть:
```
POST to: http://YOUR-IP:80/api/lora
Curl: curl -X POST http://YOUR-IP:80/api/lora -H "Content-Type: application/json" -d '{"user_id":"esp32","cold":1,"hot":0}'
POST #1 successful (cold=1)
```

## Откат изменений

Если что-то пошло не так:

```bash
sudo cp /etc/nginx/sites-available/lora-traffic-tester.backup /etc/nginx/sites-available/lora-traffic-tester
sudo systemctl reload nginx
```

## Заключение

После применения этих изменений:
- ESP32 может отправлять POST на `http://IP/api/lora`
- HTTPS остаётся для веб-интерфейса
- CORS настроен для кросс-доменных запросов
- Логи ведутся отдельно для API
