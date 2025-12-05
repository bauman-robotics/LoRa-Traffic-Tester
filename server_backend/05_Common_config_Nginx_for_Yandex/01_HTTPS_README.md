# Доработка конфигурации для поддержки HTTPS

## Задача

Дополнить существующую конфигурацию Nginx поддержкой HTTPS-запросов, чтобы обеспечить безопасное соединение для клиентов.

## Необходимые доработки

### 1. Получение SSL-сертификата

- **Рекомендуемый способ:** Использовать Let's Encrypt (бесплатный сертификат)
- **Альтернативы:** Самоподписанный сертификат (для тестирования) или коммерческий сертификат

#### Установка Certbot для Let's Encrypt:
```bash
sudo apt update
sudo apt install certbot python3-certbot-nginx
```

#### Получение сертификата:
```bash
sudo certbot --nginx -d xx-xx-xx-xx.nip.io
```

**Примечание:** Замените `xx-xx-xx-xx` на IP-адрес вашего сервера с дефисами (например, для 192.168.1.100 используйте `192-168-1-100.nip.io`). nip.io позволяет использовать IP-адрес как домен для тестирования.

### 2. Обновление конфигурации Nginx

Добавить в файл `/etc/nginx/sites-available/lora-api_php-apps_unified` новый server-блок для HTTPS:

```nginx
server {
    listen 443 ssl http2;
    server_name xx-xx-xx-xx.nip.io;

    # SSL настройки
    ssl_certificate /etc/letsencrypt/live/xx-xx-xx-xx.nip.io/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/xx-xx-xx-xx.nip.io/privkey.pem;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers ECDHE-RSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384;
    ssl_prefer_server_ciphers off;

    # Прокси для Flask API
    location /api/ {
        proxy_pass http://127.0.0.1:5001;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
    }

    # Обработка PHP скриптов
    location ~ \.php$ {
        include snippets/fastcgi-php.conf;
        fastcgi_pass unix:/var/run/php/php8.1-fpm.sock;
        fastcgi_param SCRIPT_FILENAME $document_root$fastcgi_script_name;
        include fastcgi_params;
    }

    # Статические файлы
    location / {
        try_files $uri $uri/ =404;
    }
}
```

### 3. Перенаправление HTTP на HTTPS

Обновить существующий server-блок (порт 80) для автоматического перенаправления:

```nginx
server {
    listen 80;
    server_name xx-xx-xx-xx.nip.io;
    return 301 https://$server_name$request_uri;
}
```

### 4. Настройка файрвола

Открыть порт 443 для HTTPS:
```bash
sudo ufw allow 443
sudo ufw status
```

### 5. Автоматическое обновление сертификатов

Certbot автоматически добавляет задачу в cron для обновления сертификатов. Проверить:
```bash
sudo crontab -l | grep certbot
```

### 6. Тестирование конфигурации

1. Проверить синтаксис Nginx:
   ```bash
   sudo nginx -t
   ```

2. Перезагрузить Nginx:
   ```bash
   sudo systemctl reload nginx
   ```

3. Протестировать HTTPS:
   ```bash
   curl -I https://xx-xx-xx-xx.nip.io
   ```

## Структура с HTTPS

```
Клиент (HTTPS/HTTP)
    |
    +--> HTTP --> Nginx (перенаправление на HTTPS)
    |
    +--> HTTPS --> Nginx (порт 443, SSL)
                  |
                  +--> PHP-FPM (PHP скрипты)
                  |
                  +--> Reverse Proxy --> Gunicorn --> Flask API
```

## Важные замечания

- nip.io автоматически разрешает домены на основе IP-адресов (например, `192-168-1-100.nip.io` разрешается в `192.168.1.100`)
- Для production рекомендуется использовать HSTS заголовки
- Регулярно проверяйте срок действия сертификатов
- Мониторьте логи Nginx на ошибки SSL
