# Шаги по применению новой конфигурации Nginx

## Полная последовательность действий

### 1. Создание/замена конфигурации
```bash
# Отредактировать или заменить файл конфигурации
sudo nano /etc/nginx/sites-available/lora-api

# Или скопировать готовую конфигурацию
sudo cp ~/lora-traffic-tester-complete.conf /etc/nginx/sites-available/lora-api
```

### 2. Проверка синтаксиса (ОБЯЗАТЕЛЬНО!)
```bash
# Проверить корректность конфигурации
sudo nginx -t

# Ожидаемый вывод:
# nginx: the configuration file /etc/nginx/nginx.conf syntax is ok
# nginx: configuration file /etc/nginx/nginx.conf test is successful
```

### 3. Перезагрузка Nginx
```bash
# Применить изменения (только если синтаксис корректен!)
sudo systemctl reload nginx

# Для полной перезагрузки (если reload не работает):
# sudo systemctl restart nginx
```

### 4. Проверка статуса
```bash
# Проверить, что Nginx работает
sudo systemctl status nginx

# Ожидаемый вывод: Active: active (running)
```

### 5. Проверка активной конфигурации
```bash
# Посмотреть загруженные server блоки
sudo nginx -T | grep -A 10 "server {"

# Проверить, что оба сервера (80 и 443) загружены
sudo nginx -T | grep "listen"
```

### 6. Тестирование API
```bash
# Тест HTTP API (для ESP32)
curl -X POST http://YOUR-SERVER-IP/api/lora \
  -H "Content-Type: application/json" \
  -d '{"user_id":"test","cold":1,"hot":0,"alarm_time":100}'

# Ожидаемый ответ: HTTP 200 с JSON от Flask API

# Тест HTTPS API
curl -X POST https://your-server-ip.nip.io/api/lora \
  -H "Content-Type: application/json" \
  -d '{"user_id":"test","cold":1,"hot":0,"alarm_time":100}'

# Тест CORS (для ESP32)
curl -X OPTIONS http://YOUR-SERVER-IP/api/lora \
  -H "Origin: http://esp32.local"
# Должен вернуть Access-Control-* заголовки
```

### 7. Проверка логов
```bash
# Логи API запросов
sudo tail -f /var/log/nginx/api_access.log

# Логи ошибок
sudo tail -f /var/log/nginx/error.log

# Логи HTTP API (отдельные)
sudo tail -f /var/log/nginx/http_api_access.log
```

## Диагностика проблем

### Если синтаксис некорректен:
```bash
# Посмотреть детальную ошибку
sudo nginx -t

# Проверить конкретную строку
sudo nginx -c /etc/nginx/nginx.conf -T 2>&1 | head -20
```

### Если конфигурация не применяется:
```bash
# Проверить, что файл в sites-enabled
ls -la /etc/nginx/sites-enabled/

# Создать symlink если нужно
sudo ln -s /etc/nginx/sites-available/lora-api /etc/nginx/sites-enabled/

# Перезапустить полностью
sudo systemctl restart nginx
```

### Если API не отвечает:
```bash
# Проверить Flask API
sudo systemctl status lora-api

# Логи Flask
sudo journalctl -u lora-api -f

# Тест прямого подключения к Flask
curl http://127.0.0.1:5001/api/health
```

### Если CORS не работает:
```bash
# Проверить заголовки
curl -I -X OPTIONS http://YOUR-SERVER-IP/api/lora

# Должны быть:
# Access-Control-Allow-Origin: *
# Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS
```

## Мониторинг после развертывания

### Проверка открытых портов:
```bash
sudo netstat -tlnp | grep nginx
# Должны быть: 0.0.0.0:80 и 0.0.0.0:443
```

### Проверка SSL:
```bash
# Тест SSL сертификата
openssl s_client -connect your-server-ip.nip.io:443 -servername your-server-ip.nip.io < /dev/null

# Тест HSTS
curl -I https://your-server-ip.nip.io/api/ | grep -i strict
```

### Производительность:
```bash
# Количество активных соединений
sudo nginx -s status 2>/dev/null || echo "Nginx status not available"

# Использование ресурсов
ps aux | grep nginx
```

## Резервное копирование

### Перед изменениями:
```bash
# Сделать backup
sudo cp /etc/nginx/sites-available/lora-api /etc/nginx/sites-available/lora-api.backup.$(date +%Y%m%d_%H%M%S)
```

### Восстановление при проблемах:
```bash
# Восстановить из backup
sudo cp /etc/nginx/sites-available/lora-api.backup.* /etc/nginx/sites-available/lora-api
sudo systemctl reload nginx
```

## Автоматизация

### Скрипт для быстрого развертывания:
```bash
#!/bin/bash
# deploy_nginx.sh

set -e

echo "Проверка синтаксиса..."
sudo nginx -t

echo "Перезагрузка Nginx..."
sudo systemctl reload nginx

echo "Проверка статуса..."
sudo systemctl is-active nginx

echo "Тестирование API..."
curl -s -X POST http://YOUR-SERVER-IP/api/lora \
  -H "Content-Type: application/json" \
  -d '{"user_id":"deploy_test","cold":1,"hot":0}' \
  | jq .status 2>/dev/null || echo "API test completed"

echo "Развертывание завершено успешно!"
```

## Важные замечания

- **Всегда проверяйте синтаксис** перед перезагрузкой!
- **Тестируйте API** после каждого изменения
- **Мониторьте логи** для выявления проблем
- **Делайте backup** перед изменениями
- **Используйте HTTPS** для production трафика

## Контакты для поддержки

При проблемах:
1. Проверьте логи Nginx
2. Проверьте логи Flask API
3. Проверьте сетевые настройки
4. Используйте документацию в других README файлах
