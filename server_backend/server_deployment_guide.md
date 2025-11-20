# Руководство по развертыванию сервера для сбора и визуализации данных LoRa

Это руководство описывает процесс настройки сервера для приема данных от LoRa трансивера, хранения их в базе данных PostgreSQL и визуализации в Grafana.

## Требования

- Linux сервер (Ubuntu/Debian рекомендуются)
- Docker и Docker Compose (опционально, но рекомендуется)
- Python 3.8+
- PostgreSQL (если не используется Docker)

## 1. Настройка сервера

### Вариант 1: Использование Docker Compose (Рекомендуется)

Создайте файл `docker-compose.yml` в корне проекта:

```yaml
version: '3.8'

services:
  postgres:
    image: postgres:14
    container_name: lora_postgres
    environment:
      POSTGRES_DB: lora_db
      POSTGRES_USER: lora_user
      POSTGRES_PASSWORD: lora_pass
    ports:
      - "5432:5432"
    volumes:
      - postgres_data:/var/lib/postgresql/data

  grafana:
    image: grafana/grafana:latest
    container_name: lora_grafana
    environment:
      GF_SECURITY_ADMIN_USER: admin
      GF_SECURITY_ADMIN_PASSWORD: admin
    ports:
      - "3000:3000"
    volumes:
      - grafana_data:/var/lib/grafana
    depends_on:
      - postgres

  data_collector:
    build: .
    container_name: lora_data_collector
    environment:
      DATABASE_URL: postgresql://lora_user:lora_pass@postgres:5432/lora_db
    ports:
      - "5000:5000"
    depends_on:
      - postgres

volumes:
  postgres_data:
  grafana_data:
```

Создайте `Dockerfile` для Python приложения:

```dockerfile
FROM python:3.9-slim

WORKDIR /app

COPY requirements_server.txt .
RUN pip install -r requirements_server.txt

COPY data_collector.py .

EXPOSE 5000

CMD ["python", "data_collector.py"]
```

### Вариант 2: Ручная установка

Обновите систему и установите необходимые пакеты:

```bash
sudo apt update
sudo apt install -y python3 python3-pip postgresql postgresql-contrib
```

## 2. Настройка PostgreSQL

### При использовании Docker

База данных будет автоматически создана Docker Compose.

### Ручная настройка

Создайте базу данных и пользователя:

```bash
sudo -u postgres psql

CREATE DATABASE lora_db;
CREATE USER lora_user WITH ENCRYPTED PASSWORD 'lora_pass';
GRANT ALL PRIVILEGES ON DATABASE lora_db TO lora_user;

\q
```

Создайте таблицу для хранения данных:

Подключитесь к базе данных:

```bash
psql -h localhost -U lora_user -d lora_db
```

Создайте таблицу:

```sql
CREATE TABLE lora_packets (
    id SERIAL PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    data TEXT NOT NULL,
    rssi INTEGER,
    snr FLOAT,
    frequency FLOAT,
    bandwidth INTEGER,
    spreading_factor INTEGER
);

CREATE INDEX idx_timestamp ON lora_packets(timestamp);
```

## 3. Скрипт для сбора данных

Создайте файл `data_collector.py`:

```python
from flask import Flask, request, jsonify
import psycopg2
import os
from datetime import datetime

app = Flask(__name__)

def get_db_connection():
    return psycopg2.connect(
        host=os.environ.get('DB_HOST', 'localhost'),
        database=os.environ.get('DB_NAME', 'lora_db'),
        user=os.environ.get('DB_USER', 'lora_user'),
        password=os.environ.get('DB_PASS', 'lora_pass')
    )

@app.route('/lora', methods=['POST'])
def receive_lora_data():
    try:
        data = request.get_json()

        # Извлечение данных
        payload = data.get('data', '')
        rssi = data.get('rssi')
        snr = data.get('snr')
        frequency = data.get('frequency')
        bandwidth = data.get('bandwidth')
        sf = data.get('spreading_factor')

        # Сохранение в базу данных
        conn = get_db_connection()
        cur = conn.cursor()

        cur.execute(
            """
            INSERT INTO lora_packets (data, rssi, snr, frequency, bandwidth, spreading_factor)
            VALUES (%s, %s, %s, %s, %s, %s)
            """,
            (payload, rssi, snr, frequency, bandwidth, sf)
        )

        conn.commit()
        cur.close()
        conn.close()

        return jsonify({'status': 'success'}), 200

    except Exception as e:
        print(f"Error: {e}")
        return jsonify({'status': 'error', 'message': str(e)}), 500

@app.route('/health', methods=['GET'])
def health_check():
    return jsonify({'status': 'healthy'}), 200

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
```

Создайте файл `requirements_server.txt`:

```
flask
psycopg2-binary
```

## 4. Настройка Grafana

### При использовании Docker

Grafana будет доступна по адресу `http://localhost:3000`
Логин/Пароль: admin/admin

### Ручная установка

Скачайте и установите Grafana:

```bash
wget -q -O - https://packages.grafana.com/gpg.key | sudo apt-key add -
echo "deb https://packages.grafana.com/oss/deb stable main" | sudo tee /etc/apt/sources.list.d/grafana.list
sudo apt update
sudo apt install grafana
sudo systemctl start grafana-server
sudo systemctl enable grafana-server
```

Grafana будет доступна по адресу `http://localhost:3000`

### Настройка источника данных

1. Войдите в Grafana
2. Перейдите в Configuration → Data Sources
3. Добавьте новый источник данных PostgreSQL:
   - Name: LoRa Database
   - Host: localhost:5432 (или postgres:5432 для Docker)
   - Database: lora_db
   - User: lora_user
   - Password: lora_pass

### Создание дашборда

1. Перейдите в Create → Dashboard
2. Добавьте панели для визуализации:
   - Количество пакетов за время
   - RSSI по времени
   - SNR по времени
   - Распределение spreading factor

Пример запроса для количества пакетов:

```sql
SELECT
  $__timeGroup(timestamp, '1m') as time,
  count(*) as packets
FROM lora_packets
WHERE $__timeFilter(timestamp)
GROUP BY time
ORDER BY time
```

Для RSSI:

```sql
SELECT
  timestamp as time,
  rssi
FROM lora_packets
WHERE $__timeFilter(timestamp)
ORDER BY time
```

## 5. Запуск системы

### Использование Docker Compose

```bash
docker-compose up -d
```

### Ручной запуск

1. Запустите PostgreSQL:
   ```bash
   sudo systemctl start postgresql
   ```

2. Запустите скрипт сбора данных:
   ```bash
   python3 data_collector.py
   ```

3. Запустите Grafana:
   ```bash
   sudo systemctl start grafana-server
   ```

## 6. Настройка LoRa трансивера

В файле `config.json` LoRa проекта настройте:

```json
{
  "wifi_enabled": true,
  "post_mode": "time",
  "status_enabled": true,
  "status_interval": "30",
  "log_short": true,
  "server_url": "http://your-server-ip:5000/lora"
}
```

Замените `your-server-ip` на IP-адрес вашего сервера.

## 7. Мониторинг и отладка

### Проверка работоспособности

Проверьте endpoint здоровья:

```bash
curl http://localhost:5000/health
```

### Просмотр логов

Для развертывания на базе Docker:

```bash
docker exec lora_postgres pg_dump -U lora_user lora_db > backup.sql
```

Для ручной установки проверьте логи Flask приложения.

### Распространенные проблемы

1. **Ошибка подключения к базе данных**: Проверьте параметры подключения и доступность PostgreSQL
2. **Grafana не видит данные**: Проверьте настройки источника данных и запросы
3. **LoRa устройство не отправляет данные**: Проверьте WiFi подключение и настройки POST

## 8. Резервное копирование

Регулярно создавайте резервные копии базы данных:

```bash
pg_dump -h localhost -U lora_user lora_db > backup_$(date +%Y%m%d_%H%M%S).sql
```

Для Docker:

```bash
docker exec lora_postgres pg_dump -U lora_user lora_db > backup.sql# Руководство по развертыванию сервера для сбора и визуализации данных LoRa
