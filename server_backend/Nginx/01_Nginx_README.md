# Инструкция: Настройка Nginx для LoRa Traffic Tester

## Обзор

Nginx используется как reverse proxy с SSL termination для:
- **Grafana** (веб-интерфейс для визуализации данных)
- **LoRa Flask API** (REST API для приёма и хранения данных)

## Архитектура

```
Интернет
    ↓
Nginx (SSL termination)
    ↓
┌─────────────────┬─────────────────┐
│ Grafana        │ LoRa Flask API  │
│ (port 3000)    │ (port 5001)     │
│ localhost      │ localhost       │
└─────────────────┴─────────────────┘
    ↓
PostgreSQL (port 5432)
```

## Предварительные требования

- Ubuntu Server 20.04+
- Root или sudo доступ
- Публичный IP-адрес
- Установленные сервисы:
  - PostgreSQL
  - Grafana
  - Python 3.8+ с виртуальным окружением

## Шаг 1: Установка Nginx

```bash
sudo apt update
sudo apt install nginx -y
sudo systemctl enable nginx
sudo systemctl start nginx
```

Проверка:
```bash
sudo systemctl status nginx
```

## Шаг 2: Установка Certbot (Let's Encrypt)

```bash
sudo apt install certbot python3-certbot-nginx -y
```

## Шаг 3: Настройка домена через nip.io

Используем nip.io для бесплатного домена без регистрации:

```
IP: 192.168.1.100 → Домен: 192-168-1-100.nip.io
```

Проверка:
```bash
ping YOUR-IP-WITH-DASHES.nip.io
```

## Шаг 4: Настройка Nginx

### Основная конфигурация

```bash

sudo nano /etc/nginx/sites-available/lora-api

```

Содержимое файла:

```nginx
# Upstream для API
upstream lora_api {
    server 127.0.0.1:5001;
}

# HTTPS сервер
server {
    listen 443 ssl http2;
    server_name YOUR-IP-WITH-DASHES.nip.io;

    # SSL сертификаты
    ssl_certificate /etc/letsencrypt/live/YOUR-IP-WITH-DASHES.nip.io/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/YOUR-IP-WITH-DASHES.nip.io/privkey.pem;

    # SSL настройки
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256;
    ssl_prefer_server_ciphers off;

    # Безопасность
    add_header X-Frame-Options SAMEORIGIN;
    add_header X-Content-Type-Options nosniff;
    add_header Strict-Transport-Security "max-age=31536000; includeSubDomains" always;

    # LoRa API
    location /api/ {
        proxy_pass http://lora_api;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        # Таймауты
        proxy_connect_timeout 30;
        proxy_send_timeout 30;
        proxy_read_timeout 30;

        # CORS для ESP32
        add_header Access-Control-Allow-Origin *;
        add_header Access-Control-Allow-Methods "GET, POST, OPTIONS";
        add_header Access-Control-Allow-Headers "Content-Type";

        # Обработка OPTIONS
        if ($request_method = 'OPTIONS') {
            return 204;
        }
    }

    # Grafana
    location /grafana/ {
        proxy_pass http://127.0.0.1:3000/grafana/;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        proxy_http_version 1.1;
        proxy_set_header Upgrade $http_upgrade;
        proxy_set_header Connection "upgrade";

        # Таймауты для WebSocket
        proxy_connect_timeout 300;
        proxy_send_timeout 300;
        proxy_read_timeout 300;
        send_timeout 300;

        # Оптимизация буферов
        proxy_buffering off;
        proxy_buffer_size 128k;
        proxy_buffers 4 256k;
        proxy_busy_buffers_size 256k;

        client_max_body_size 50M;
    }

    # Редирект с корня на Grafana
    location / {
        return 301 https://$server_name/grafana/;
    }
}

# HTTP сервер (для совместимости с ESP32)
server {
    listen 80;
    server_name YOUR-IP-WITH-DASHES.nip.io;

    # API через HTTP (для ESP32 без SSL)
    location /api/ {
        proxy_pass http://lora_api;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;

        # CORS
        add_header Access-Control-Allow-Origin *;
        add_header Access-Control-Allow-Methods "GET, POST, OPTIONS";
        add_header Access-Control-Allow-Headers "Content-Type";

        if ($request_method = 'OPTIONS') {
            return 204;
        }
    }

    # Редирект остальных на HTTPS
    location / {
        return 301 https://$server_name$request_uri;
    }
}
```

### Активация конфигурации

```bash
sudo ln -s /etc/nginx/sites-available/lora-api /etc/nginx/sites-enabled/
sudo nginx -t
sudo systemctl reload nginx
```

## Шаг 5: Настройка Grafana

### Конфигурация Grafana

```bash
sudo nano /etc/grafana/grafana.ini
```

Измените секции:

```ini
[server]
protocol = http
http_addr = 127.0.0.1
http_port = 3000
domain = YOUR-IP-WITH-DASHES.nip.io
root_url = https://YOUR-IP-WITH-DASHES.nip.io/grafana/
serve_from_sub_path = true

[security]
cookie_secure = false
cookie_samesite = lax

[auth.anonymous]
enabled = true
org_role = Viewer
```

Перезапуск:
```bash
sudo systemctl restart grafana-server
```

## Шаг 6: Настройка LoRa Flask API

### Systemd сервис

```bash
sudo nano /etc/systemd/system/lora-api.service
```

Содержимое:

```ini
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

ExecStart=/root/lora_flask_api/venv/bin/gunicorn \
          --bind 127.0.0.1:5001 \
          --workers 2 \
          --timeout 60 \
          --access-logfile - \
          --error-logfile - \
          lora_api:app

Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
```

### Запуск сервиса

```bash
sudo systemctl daemon-reload
sudo systemctl enable lora-api
sudo systemctl start lora-api
sudo systemctl status lora-api
```

## Шаг 7: Получение SSL-сертификата

```bash
sudo certbot --nginx -d YOUR-IP-WITH-DASHES.nip.io
```

## Шаг 8: Проверка работы

### API тесты

```bash
# HTTP (для ESP32)
curl -X POST http://YOUR-IP/api/lora \
  -H "Content-Type: application/json" \
  -d '{"user_id":"test","cold":1,"hot":0}'

# HTTPS
curl -X POST https://YOUR-IP/api/lora \
  -H "Content-Type: application/json" \
  -d '{"user_id":"test","cold":1,"hot":0}'
```

### Grafana

Открыть: `https://YOUR-IP/grafana/`

## Шаг 9: Мониторинг и логи

### Nginx логи

```bash
sudo tail -f /var/log/nginx/access.log
sudo tail -f /var/log/nginx/error.log
```

### Сервис логи

```bash
sudo journalctl -u lora-api -f
sudo journalctl -u grafana-server -f
```

### Проверка сертификата

```bash
openssl s_client -connect YOUR-IP:443 -servername YOUR-IP
```

## Конфигурация для разработки

Для локальной разработки без SSL:

```nginx
server {
    listen 80;
    server_name localhost;

    location /api/ {
        proxy_pass http://127.0.0.1:5001;
    }

    location /grafana/ {
        proxy_pass http://127.0.0.1:3000/grafana/;
    }
}
```

## Troubleshooting

### 502 Bad Gateway

Проверить статус сервисов:
```bash
sudo systemctl status lora-api
sudo systemctl status grafana-server
```

### SSL ошибки

```bash
sudo certbot certificates
sudo certbot renew
```

### CORS проблемы

В Nginx добавить:
```nginx
add_header Access-Control-Allow-Origin *;
add_header Access-Control-Allow-Methods "GET, POST, OPTIONS";
add_header Access-Control-Allow-Headers "Content-Type";
```

## Производительность

### Оптимизация Nginx

В `/etc/nginx/nginx.conf`:

```nginx
worker_processes auto;
worker_connections 1024;

http {
    keepalive_timeout 300;
    client_max_body_size 50M;
    gzip on;
    gzip_types text/plain text/css application/json application/javascript text/xml application/xml application/xml+rss text/javascript;
}
```

### Оптимизация Gunicorn

В systemd сервисе увеличить workers:
```ini
--workers 4 \
--worker-class gevent \
```

## Безопасность

### Fail2Ban

```bash
sudo apt install fail2ban
sudo systemctl enable fail2ban
```

### Firewall

```bash
sudo ufw allow 80
sudo ufw allow 443
sudo ufw enable
```

## Автоматическое обновление

### SSL сертификаты

```bash
sudo systemctl status certbot.timer
```

### Система

```bash
sudo apt update && sudo apt upgrade -y
```

## Структура файлов

```
server_backend/
├── Nginx/
│   └── README.md              # Эта инструкция
├── Grafana/
│   ├── grafana_https_public_guide.md
│   └── Grafana__dashboard_pro_v1.json
└── lora_flask_api/
    ├── lora_api.py
    ├── config.py
    ├── lora-api.service.txt
    └── requirements.txt
