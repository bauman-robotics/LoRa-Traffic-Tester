# Инструкция: Настройка Grafana через HTTPS с Let's Encrypt

## Предварительные требования

- Ubuntu Server с установленными Nginx и Grafana
- Публичный IP-адрес сервера
- Root или sudo доступ

---

## Шаг 1: Установка Certbot для Let's Encrypt

```bash
sudo apt update
sudo apt install certbot python3-certbot-nginx -y
```

---

## Шаг 2: Настройка домена через nip.io

Используем бесплатный сервис nip.io, который автоматически резолвит IP в домен без необходимости покупки домена.

Для IP `x.x.x.x` домен будет: `x-x-x-x.nip.io` (замените точки на дефисы)

**Пример:**
- IP: `192.168.1.100` → Домен: `192-168-1-100.nip.io`
- IP: `10.0.0.50` → Домен: `10-0-0-50.nip.io`

Проверьте, что домен работает:
```bash
ping YOUR-IP-WITH-DASHES.nip.io
```

---

## Шаг 3: Настройка Nginx

Отредактируйте конфигурацию Nginx:

```bash
sudo nano /etc/nginx/sites-available/lora-api
```

Пример конфигурации:

```nginx
server {
    listen 443 ssl;
    server_name YOUR-IP-WITH-DASHES.nip.io;
    
    # Сертификаты (будут обновлены Certbot автоматически)
    ssl_certificate /etc/letsencrypt/live/YOUR-IP-WITH-DASHES.nip.io/fullchain.pem;
    ssl_certificate_key /etc/letsencrypt/live/YOUR-IP-WITH-DASHES.nip.io/privkey.pem;
    ssl_protocols TLSv1.2 TLSv1.3;
    ssl_ciphers ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES128-GCM-SHA256;
    ssl_prefer_server_ciphers off;
    
    add_header X-Frame-Options SAMEORIGIN;
    add_header X-Content-Type-Options nosniff;
    
    # Ваш основной API (опционально)
    location / {
        proxy_pass http://127.0.0.1:YOUR_API_PORT;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
        proxy_set_header X-Forwarded-For $proxy_add_x_forwarded_for;
        proxy_set_header X-Forwarded-Proto $scheme;
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
        
        # Увеличенные таймауты для предотвращения обрыва соединения
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
}

server {
    listen 80;
    server_name YOUR-IP-WITH-DASHES.nip.io;
    return 301 https://$server_name$request_uri;
}
```

Проверьте и перезагрузите Nginx:

```bash
sudo nginx -t
sudo systemctl reload nginx
```

---

## Шаг 4: Настройка глобальных параметров Nginx

Отредактируйте основной конфиг:

```bash
sudo nano /etc/nginx/nginx.conf
```

В секции `http {` добавьте или измените:

```nginx
http {
    # Существующие настройки...
    
    # Увеличенные таймауты
    keepalive_timeout 300;
    client_body_timeout 300;
    client_header_timeout 300;
    send_timeout 300;
    
    # Увеличенные размеры буферов
    client_max_body_size 50M;
    client_body_buffer_size 128k;
    
    # Остальные настройки...
}
```

Перезагрузите Nginx:

```bash
sudo nginx -t
sudo systemctl reload nginx
```

---

## Шаг 5: Получение SSL-сертификата от Let's Encrypt

```bash
sudo certbot --nginx -d YOUR-IP-WITH-DASHES.nip.io
```

Certbot спросит:
1. **Email**: введите ваш email для уведомлений
2. **Terms of Service**: согласитесь (Y)
3. **Share email**: можете отказаться (N)

Certbot автоматически:
- Получит сертификат
- Обновит конфигурацию Nginx
- Настроит автоматическое обновление

Проверьте автоматическое обновление:

```bash
sudo systemctl status certbot.timer
```

---

## Шаг 6: Настройка Grafana

Отредактируйте конфигурацию Grafana:

```bash
sudo nano /etc/grafana/grafana.ini
```

Найдите и измените секцию `[server]`:

```ini
[server]
protocol = http
http_addr = 127.0.0.1
http_port = 3000
domain = YOUR-IP-WITH-DASHES.nip.io
root_url = https://YOUR-IP-WITH-DASHES.nip.io/grafana/
serve_from_sub_path = true
```

Найдите и измените секцию `[security]`:

```ini
[security]
cookie_secure = false
cookie_samesite = lax
allow_embedding = false
```

Найдите и измените секцию `[auth.anonymous]` (для возможности делиться ссылками):

```ini
[auth.anonymous]
# enable anonymous access
enabled = true

# specify role for unauthenticated users
org_role = Viewer
```

**Примечание:** Если вам НЕ нужно делиться публичными ссылками на дашборды, установите `enabled = false`.

Перезапустите Grafana:

```bash
sudo systemctl restart grafana-server
sudo systemctl status grafana-server
```

---

## Шаг 7: Проверка работы

Проверьте доступ через командную строку:

```bash
curl -I https://YOUR-IP-WITH-DASHES.nip.io/grafana/
```

Должен вернуться `HTTP/1.1 302 Found` с редиректом на `/grafana/login`.

Откройте в браузере:
```
https://YOUR-IP-WITH-DASHES.nip.io/grafana/
```

**Логин по умолчанию**: `admin`  
**Пароль по умолчанию**: `admin`

При первом входе Grafana попросит сменить пароль.

---

## Шаг 8: Сброс пароля admin (если необходимо)

Если забыли пароль или он не работает:

```bash
# Остановите Grafana
sudo systemctl stop grafana-server

# Сбросьте пароль через базу данных (пароль будет: admin)
sudo sqlite3 /var/lib/grafana/grafana.db "UPDATE user SET password = '59acf18b94d7eb0694c61e60ce44c110c7a683ac6a8f09580d626f90f4a242000746579358d77dd9e570e83fa24faa88a8a6', salt = 'F3FAxVm33R' WHERE login = 'admin';"

# Запустите Grafana
sudo systemctl start grafana-server
```

Войдите с `admin` / `admin` и смените пароль.

---

## Автоматическое обновление сертификата

Let's Encrypt сертификаты действительны 90 дней. Certbot автоматически настраивает обновление через systemd timer.

Проверьте статус:

```bash
sudo systemctl status certbot.timer
```

Протестируйте обновление:

```bash
sudo certbot renew --dry-run
```

---

## Проверка логов

**Логи Grafana:**
```bash
sudo tail -f /var/log/grafana/grafana.log
```

**Логи Nginx:**
```bash
sudo tail -f /var/log/nginx/error.log
sudo tail -f /var/log/nginx/access.log
```

---

# Подводные камни и решения

## 1. Ошибка 400 Bad Request

**Проблема:** Grafana настроена на HTTPS (`protocol = https`), а Nginx подключается по HTTP.

**Симптомы:**
- `curl` возвращает `400 Bad Request`
- В логах Grafana: `TLS handshake error ... client sent an HTTP request to an HTTPS server`

**Решение:** В `grafana.ini` установить `protocol = http`. 

**Объяснение:** Grafana должна работать по HTTP локально, а SSL терминируется на Nginx. Nginx подключается к Grafana по HTTP на localhost, а внешние клиенты получают HTTPS.

---

## 2. Ошибка 301 Moved Permanently (редирект на тот же URL)

**Проблема:** Неправильные настройки cookie в Grafana.

**Симптомы:**
- `curl` показывает `301 Moved Permanently` с `Location` на тот же URL
- Бесконечный редирект в браузере

**Решение:**
```ini
[security]
cookie_secure = false  # НЕ true!
cookie_samesite = lax  # НЕ none
```

**Объяснение:** `cookie_secure = true` требует HTTPS соединения. Но Nginx подключается к Grafana по HTTP, поэтому нужно `false`.

---

## 3. "Grafana has failed to load its application files"

**Проблема:** Неправильный `root_url` в конфигурации Grafana.

**Симптомы:**
- HTML страница загружается
- Статические файлы (CSS/JS) не загружаются
- Сообщение об ошибке загрузки приложения

**Частые причины:**
- Использование переменных `%(protocol)s://%(domain)s:%(http_port)s/grafana/` с неправильным портом
- Отсутствие `serve_from_sub_path = true`

**Решение:** Использовать полный URL без порта:
```ini
[server]
root_url = https://YOUR-DOMAIN.nip.io/grafana/
serve_from_sub_path = true
```

---

## 4. Статические файлы требуют авторизацию (302 редирект)

**Проблема:** Grafana требует авторизацию даже для публичных CSS/JS файлов.

**Симптомы:**
- `curl -I https://domain/grafana/public/build/runtime.js` возвращает `302 Found`
- Браузер показывает ошибку загрузки статических ресурсов

**Решение:** Включить анонимный доступ в `grafana.ini`:
```ini
[auth.anonymous]
enabled = true
org_role = Viewer
```

**Альтернатива:** Настроить отдельный location в Nginx для `/grafana/public/`, но это сложнее.

---

## 5. NS_ERROR_NET_PARTIAL_TRANSFER в браузере

**Проблема:** Соединение обрывается при загрузке больших файлов из-за таймаутов.

**Симптомы:**
- В Network вкладке браузера: `NS_ERROR_NET_PARTIAL_TRANSFER`
- Файлы загружаются частично
- Grafana показывает ошибку загрузки

**Решение:** Увеличить таймауты и размеры буферов в Nginx.

В `sites-available/YOUR-CONFIG`:
```nginx
location /grafana/ {
    # ... другие настройки ...
    
    proxy_connect_timeout 300;
    proxy_send_timeout 300;
    proxy_read_timeout 300;
    send_timeout 300;
    
    proxy_buffering off;
    proxy_buffer_size 128k;
    proxy_buffers 4 256k;
    proxy_busy_buffers_size 256k;
    
    client_max_body_size 50M;
}
```

В `nginx.conf`:
```nginx
http {
    keepalive_timeout 300;
    client_body_timeout 300;
    client_header_timeout 300;
    send_timeout 300;
    
    client_max_body_size 50M;
    client_body_buffer_size 128k;
}
```

---

## 6. Кэширование браузера

**Проблема:** Браузер кэширует старые версии файлов после изменения конфигурации.

**Симптомы:**
- Изменения не применяются
- Старая ошибка остаётся даже после исправления
- Работает на другом устройстве, но не на текущем

**Решение:**

**Firefox:**
1. Меню (≡) → Settings → Privacy & Security
2. Cookies and Site Data → Clear Data
3. Выбрать оба пункта → Clear

**Chrome:**
1. Settings → Privacy and security → Clear browsing data
2. Cached images and files → Clear data

**Универсально:**
- Режим инкогнито: `Ctrl + Shift + P` (Firefox) или `Ctrl + Shift + N` (Chrome)
- Принудительная перезагрузка: `Ctrl + Shift + R` или `Ctrl + F5`

---

## 7. Неправильный proxy_pass в Nginx

**Проблема:** Использование `proxy_pass http://127.0.0.1:3000/` (со слэшем в конце) без учета subpath.

**Симптомы:**
- 404 ошибки на некоторых путях
- Неправильная маршрутизация запросов

**Решение:** Для работы с subpath использовать:
```nginx
location /grafana/ {
    proxy_pass http://127.0.0.1:3000/grafana/;
    # ...
}
```

**Правило:** 
- `location /grafana/` + `proxy_pass http://...:3000/grafana/;` — путь сохраняется
- `location /grafana/` + `proxy_pass http://...:3000;` — путь заменяется

Слэш в конце **критически важен** для правильной работы с путями.

---

## 8. Дублирующиеся настройки в grafana.ini

**Проблема:** В файле конфигурации могут быть несколько строк с одной настройкой.

**Симптомы:**
- Настройки не применяются
- Поведение Grafana отличается от ожидаемого

**Объяснение:** В `.ini` файлах:
- `;настройка = значение` — закомментировано (неактивно)
- `настройка = значение` — активно

**Решение:** Проверить активные настройки:
```bash
grep -E "^(protocol|root_url|serve_from_sub_path|cookie_secure)" /etc/grafana/grafana.ini
```

Убедиться, что нет конфликтующих значений и что активна только одна строка для каждой настройки.

---

## 9. Забытый пароль admin

**Проблема:** Пароль по умолчанию не работает или был изменен и забыт.

**Решение:** Сброс через SQLite базу данных:
```bash
sudo systemctl stop grafana-server
sudo sqlite3 /var/lib/grafana/grafana.db "UPDATE user SET password = '59acf18b94d7eb0694c61e60ce44c110c7a683ac6a8f09580d626f90f4a242000746579358d77dd9e570e83fa24faa88a8a6', salt = 'F3FAxVm33R' WHERE login = 'admin';"
sudo systemctl start grafana-server
```

После этого: Логин `admin`, Пароль `admin`.

**Альтернатива через CLI** (не всегда работает):
```bash
sudo grafana-cli admin reset-admin-password newpassword
```

---

## 10. Статические файлы возвращают 404

**Проблема:** Файлы типа `/grafana/public/build/...` не найдены.

**Симптомы:**
- `curl -I https://domain/grafana/public/build/file.js` → `404 Not Found`
- Grafana показывает белый экран или ошибку загрузки

**Возможные причины:**
- Неправильный `proxy_pass` (см. проблему #7)
- Отсутствие `serve_from_sub_path = true`
- Требуется авторизация (см. проблему #4)

**Диагностика:**
```bash
# Проверить прямой доступ к Grafana
curl -I http://127.0.0.1:3000/grafana/public/build/runtime.js

# Проверить через Nginx
curl -I https://YOUR-DOMAIN/grafana/

