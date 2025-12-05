7. Тестирование и запуск
Поэтапное тестирование:
bash

# 1. Проверить конфигурацию Nginx
sudo nginx -t

# 2. Проверить PHP-FPM
sudo php-fpm8.1 -t

# 3. Создать тестовые файлы
echo "<?php echo 'Nginx + PHP-FPM работает'; ?>" | sudo tee /var/www/html/test-nginx.php
echo "<?php phpinfo(); ?>" | sudo tee /var/www/html/phpinfo.php

# 4. Остановить Apache (временно для теста)
sudo systemctl stop apache2

# 5. Запустить Nginx
sudo systemctl start nginx
sudo systemctl enable nginx

# 6. Проверить доступность
curl -I http://localhost/test-nginx.php
curl http://localhost/phpinfo.php | head -50

Тестирование существующих PHP скриптов:
bash

# Создать скрипт для проверки всех PHP файлов
cat > /tmp/test_php.sh << 'EOF'
#!/bin/bash
echo "=== Тестирование PHP скриптов ==="
for file in /var/www/html/*.php; do
    if [ -f "$file" ]; then
        echo "Тестируем: $(basename $file)"
        php -l "$file"
        if [ $? -eq 0 ]; then
            echo "✓ OK: $(basename $file)"
        else
            echo "✗ ОШИБКА: $(basename $file)"
        fi
    fi
done
EOF

*/

chmod +x /tmp/test_php.sh
sudo /tmp/test_php.sh

=====
# Проверить что порт 80 освободился
sudo netstat -tulpn | grep :80

Запуск всего стека:
bash

# 1. Запустить PHP-FPM
sudo systemctl restart php8.1-fpm

# 2. Запустить Nginx
sudo systemctl restart nginx

# 3. Запустить Gunicorn (Flask API)
sudo systemctl start lora-api
sudo systemctl enable lora-api

# 4. Проверить все службы
sudo systemctl status php8.1-fpm nginx lora-api


# Проверить сокет PHP-FPM
ls -la /run/php/php8.1-fpm.sock

# Проверить права
sudo chown www-data:www-data /run/php/php8.1-fpm.sock

# Перезапустить PHP-FPM
sudo systemctl restart php8.1-fpm

# Проверить root директорию в Nginx конфиге
# Проверить права на файлы
sudo chown -R www-data:www-data /var/www/html
sudo chmod -R 755 /var/www/html

# Увеличить лимиты в PHP-FPM
sudo nano /etc/php/8.1/fpm/pool.d/www.conf
# Увеличить: pm.max_children, request_terminate_timeout


# 1. Остановить Nginx и PHP-FPM
sudo systemctl stop nginx php8.1-fpm lora-api

# 2. Запустить Apache
sudo systemctl start apache2
sudo systemctl enable apache2

# 3. Восстановить конфигурацию из бэкапа
sudo cp -r /etc/apache2.backup/* /etc/apache2/
sudo systemctl restart apache2

*/

Проверить логи: /var/log/nginx/error.log

Проверить логи PHP-FPM: /var/log/php8.1-fpm.log

Запустить проверку: sudo /usr/local/bin/check-web-stack.sh