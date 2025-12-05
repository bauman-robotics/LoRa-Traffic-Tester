серверная часть flask api запускается через gunicorn  + добавлен nxinx 


Роли каждого компонента:

    Flask — фреймворк для создания приложения (ваш API)

    Gunicorn — WSGI-сервер для запуска Flask

        Запускает несколько воркеров для обработки запросов

        Управляет процессами

        Обрабатывает базовую балансировку нагрузки между воркерами

    Nginx — фронтенд-веб-сервер и обратный прокси:

        Принимает HTTP/HTTPS запросы от клиентов

        Отдает статические файлы (намного быстрее, чем через Flask)

        Терминирует SSL/TLS (обрабатывает HTTPS)

        Балансирует нагрузку между Gunicorn-воркерами

        Защищает от медленных клиентов (Slowloris-атаки)

        Кэширование ответов

        Компрессия (gzip)

        Ограничение запросов (rate limiting)

Типичный flow запроса:
text

Клиент → Nginx (порт 80/443) → Gunicorn (порт 8000/9000) → Flask app


Запуск Gunicorn:
bash

gunicorn -w 4 -b 127.0.0.1:8000 "app:create_app()"

где -w 4 — количество воркеров (рекомендуется 2 * cores + 1)

Альтернативы и улучшения:

    Вместо Gunicorn можно использовать:

        uWSGI — более функциональный, но сложнее в настройке

        Waitress — чисто питоновское решение, проще

    Для супер-нагрузки можно добавить:

        Supervisor или systemd для управления процессами

        Docker для контейнеризации

        Кластер из нескольких серверов с балансировщиком

    Для микросервисов:

        Можно рассмотреть Gunicorn + Nginx + Docker Compose

        Или перейти на Kubernetes с ingress-контроллером


Проверьте эти моменты:

✅ Nginx настроен как reverse proxy к Gunicorn
✅ Gunicorn запущен на localhost (не на 0.0.0.0 если перед ним nginx)
✅ Статические файлы отдаются через Nginx
✅ Настроены правильные заголовки (X-Forwarded-For для IP клиентов)
✅ Есть systemd unit или supervisor для автозапуска Gunicorn
✅ Настроен HTTPS в Nginx (Let's Encrypt/certbot)        

====
Установка  



2. Проверка установки
bash

# Проверьте версию
gunicorn --version

# Или получите справку
gunicorn --help

3. Базовый запуск Flask приложения

Предположим, у вас есть файл app.py с Flask приложением:
python

# app.py
from flask import Flask
app = Flask(__name__)

@app.route('/')
def hello():
    return 'Hello, World!'

Запуск:
bash

# Самый простой способ (будет слушать на 127.0.0.1:8000)
gunicorn app:app
# или если app у вас в переменной application
gunicorn app:application

4. Типичные параметры запуска
bash

# Указать количество воркеров и порт
gunicorn -w 4 -b 127.0.0.1:8000 app:app

# Запуск на всех интерфейсах (не рекомендуется без reverse proxy)
gunicorn -w 4 -b 0.0.0.0:8000 app:app

# Использовать другой worker class (для асинхронных приложений)
gunicorn -w 4 -k gevent app:app

# Указать таймауты  ==============================================================!!!!!
gunicorn --timeout 120 --graceful-timeout 120 app:app

# Запуск в демоне (фоновом режиме)
gunicorn -w 4 -b 127.0.0.1:8000 --daemon app:app


6. Конфигурационный файл (рекомендуется для продакшена)

Создайте файл gunicorn.conf.py:
python

# gunicorn.conf.py
import multiprocessing

# Количество воркеров
workers = multiprocessing.cpu_count() * 2 + 1

# Адрес и порт
bind = "127.0.0.1:8000"

# Worker class
worker_class = "sync"  # или "gevent", "eventlet"

# Таймауты
timeout = 120
keepalive = 5

# Логи
accesslog = "-"  # stdout
errorlog = "-"   # stdout
loglevel = "info"

# Перезапуск при изменении кода (только для разработки!)
reload = False


Запуск с конфигурационным файлом:
bash

gunicorn -c gunicorn.conf.py app:app

7. Запуск через systemd (для автозапуска)

Создайте файл службы /etc/systemd/system/myapp.service:
ini

[Unit]
Description=Gunicorn instance для моего Flask приложения
After=network.target

[Service]
User=www-data
Group=www-data
WorkingDirectory=/path/to/your/app
Environment="PATH=/path/to/your/venv/bin"
ExecStart=/path/to/your/venv/bin/gunicorn -w 4 -b 127.0.0.1:8000 app:app

[Install]
WantedBy=multi-user.target

# Перезагрузите systemd
sudo systemctl daemon-reload

# Включите автозапуск
sudo systemctl enable myapp

# Запустите сервис
sudo systemctl start myapp

# Проверьте статус
sudo systemctl status myapp

10. Полный пример с Nginx
bash

# Установите
pip install gunicorn

# Запустите временно
gunicorn -w 4 -b 127.0.0.1:8000 app:app

# В другом терминале проверьте
curl http://127.0.0.1:8000/

# Теперь настройте nginx как описано ранее

Советы:

    Количество воркеров: workers = (2 * cores) + 1

    Никогда не запускайте Gunicorn от root в продакшене

    Всегда используйте reverse proxy (Nginx/Apache) перед Gunicorn

    Для асинхронных операций используйте gevent или eventlet

    Для CPU-bound задач используйте sync workers