# Настройка базы данных PostgreSQL для LoRa API

## Установка и базовые команды PostgreSQL

### 1. Переключиться на пользователя postgres
```bash
sudo -i -u postgres
```

### 2. Запустить psql
Теперь вы в интерактивной консоли PostgreSQL.
```bash
psql
```

### 3. Основные команды PostgreSQL
```sql
\l                    -- список баз данных
\c dbname             -- подключиться к базе
\dt                   -- показать таблицы в текущей базе
\q                    -- выйти из psql
```
```bash
exit                  -- выход из пользователя postgres
```

## Создание базы данных и пользователя

### 4. Выполняем команды последовательно:
```sql
CREATE DATABASE LoRa_db
    ENCODING 'UTF8'
    LC_COLLATE 'en_US.UTF-8'
    LC_CTYPE 'en_US.UTF-8'
    TEMPLATE template0;

CREATE USER xxx_user WITH PASSWORD 'xxx_password';

GRANT CONNECT, TEMPORARY ON DATABASE LoRa_db TO xxx_user;
GRANT CREATE ON DATABASE LoRa_db TO xxx_user;
```

### 5. Подключиться к базе
```sql
\c LoRa_db
```

## Создание таблицы

### 6. Создаем таблицу lora_tab
```sql
CREATE TABLE lora_tab (
    line_num SERIAL PRIMARY KEY,
    user_id CHARACTER VARYING(80),
    user_location CHARACTER VARYING(80),
    cold INTEGER,
    hot INTEGER,
    alarm_time INTEGER,
    destination_nodeid TEXT,
    sender_nodeid TEXT,
    packet_id INTEGER,
    header_flags SMALLINT,
    channel_hash SMALLINT,
    next_hop SMALLINT,
    relay_node SMALLINT,
    packet_data BYTEA,
    signal_level_dbm INTEGER,
    full_packet_len INTEGER,
    additional_field3 INTEGER,
    additional_field4 INTEGER,
    created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP
);
```

**Альтернативный способ создания таблицы одной командой:**
```bash
sudo -u postgres psql -d lora_db -c "CREATE TABLE lora_tab (line_num SERIAL PRIMARY KEY, user_id CHARACTER VARYING(80), user_location CHARACTER VARYING(80), cold INTEGER, hot INTEGER, alarm_time INTEGER, destination_nodeid TEXT, sender_nodeid TEXT, packet_id INTEGER, header_flags SMALLINT, channel_hash SMALLINT, next_hop SMALLINT, relay_node SMALLINT, packet_data BYTEA, signal_level_dbm INTEGER, full_packet_len INTEGER, additional_field3 INTEGER, additional_field4 INTEGER, created_at TIMESTAMP WITH TIME ZONE DEFAULT CURRENT_TIMESTAMP);"
```

## Настройка привилегий

### 7. Привилегии
```sql
GRANT ALL PRIVILEGES ON TABLE lora_tab TO xxx_user;
GRANT ALL PRIVILEGES ON SEQUENCE LoRa_tab_line_num_seq TO xxx_user;
```

**Альтернативный способ настройки привилегий:**
```bash
sudo -u postgres psql -d lora_db -c "GRANT ALL PRIVILEGES ON TABLE lora_tab TO xxx_user; GRANT ALL PRIVILEGES ON SEQUENCE LoRa_tab_line_num_seq TO xxx_user;"
```

## Структура таблицы

Итоговая структура таблицы:

```
                                              Table "public.lora_tab"
       Column       |           Type           | Collation | Nullable |                  Default
--------------------+--------------------------+-----------+----------+--------------------------------------------
 line_num           | integer                  |           | not null | nextval('lora_tab_line_num_seq'::regclass)
 user_id            | character varying(80)    |           |          |
 user_location      | character varying(80)    |           |          |
 cold               | integer                  |           |          |
 hot                | integer                  |           |          |
 alarm_time         | integer                  |           |          |
 destination_nodeid | text                     |           |          |
 sender_nodeid      | text                     |           |          |
 packet_id          | integer                  |           |          |
 header_flags       | smallint                 |           |          |
 channel_hash       | smallint                 |           |          |
 next_hop           | smallint                 |           |          |
 relay_node         | smallint                 |           |          |
 packet_data        | bytea                    |           |          |
 signal_level_dbm   | integer                  |           |          |
 full_packet_len    | integer                  |           |          |
 additional_field3  | integer                  |           |          |
 additional_field4  | integer                  |           |          |
 created_at         | timestamp with time zone |           |          | CURRENT_TIMESTAMP
Indexes:
    "lora_tab_pkey" PRIMARY KEY, btree (line_num)
```

## Верификация настройки

### 8. Проверяем
```sql
\d lora_tab
\dt+
\c lora_db
\d lora_tab
```
В выводе должны появиться:
```
lora_db
lora_tab
```

## Настройка временной зоны

=======================================

### Установка московской временной зоны
```bash
SET timezone = 'Europe/Moscow';
```

### Глобальная настройка временной зоны
```bash
# Установим московскую временную зону
sudo timedatectl set-timezone Europe/Moscow

# Проверим
timedatectl

# Перезапустим PostgreSQL чтобы применил новую зону
sudo systemctl restart postgresql

# Перезапустим Flask API
sudo systemctl restart lora-api.service
```

===========================================
