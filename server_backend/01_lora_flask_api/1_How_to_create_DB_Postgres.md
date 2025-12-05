# Настройка базы данных PostgreSQL для LoRa API

## Краткий план настройки

Для успешной настройки базы данных PostgreSQL выполните следующие шаги:

1. **Подключиться к PostgreSQL** как пользователь postgres
2. **Создать базу данных** и пользователя с правами доступа
3. **Создать таблицу** для хранения данных LoRa пакетов
4. **Настроить права доступа** для пользователя
5. **Проверить настройку** базы данных и таблицы
6. **Настроить аутентификацию** (pg_hba.conf) для парольного доступа
7. **Настроить временную зону** (опционально)

> **ВНИМАНИЕ:** В этом руководстве используются placeholder значения, которые необходимо заменить на свои:
> - `lora_db` → ваше название базы данных
> - `xxx_user` → ваше имя пользователя базы данных
> - `xxx_password` → ваш пароль пользователя
> - `lora_tab` → ваше название таблицы (если отличается)
>
> Рекомендуется использовать более безопасные имена и пароли.

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
CREATE DATABASE lora_db
    ENCODING 'UTF8'
    LC_COLLATE 'en_US.UTF-8'
    LC_CTYPE 'en_US.UTF-8'
    TEMPLATE template0;

CREATE USER xxx_user WITH PASSWORD 'xxx_password';

GRANT CONNECT, TEMPORARY ON DATABASE lora_db TO xxx_user;
GRANT CREATE ON DATABASE lora_db TO xxx_user;
```

### 5. Подключиться к базе
```sql
\c lora_db
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
GRANT ALL PRIVILEGES ON SEQUENCE lora_tab_line_num_seq TO xxx_user;
```

**Альтернативный способ настройки привилегий:**
```bash
sudo -u postgres psql -d lora_db -c "GRANT ALL PRIVILEGES ON TABLE lora_tab TO xxx_user; GRANT ALL PRIVILEGES ON SEQUENCE lora_tab_line_num_seq TO xxx_user;"
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

### 8. Проверяем настройку
```sql
-- Показать структуру таблицы
\d lora_tab

-- Показать все таблицы в базе данных
\dt
```

В выводе должны появиться:
- Структура таблицы `lora_tab` со всеми колонками
- Таблица `lora_tab` в списке таблиц базы данных

## Настройка аутентификации PostgreSQL

### 9. Настройка pg_hba.conf для парольной аутентификации

Сначала найдем правильный путь к файлу конфигурации (версия PostgreSQL может отличаться):

```bash
# Найти файл pg_hba.conf
find /etc/postgresql -name "pg_hba.conf" 2>/dev/null

# Или проверить версию PostgreSQL и путь
ls -la /etc/postgresql/
```

Обычно файл находится в `/etc/postgresql/[версия]/main/pg_hba.conf`, где [версия] - номер версии PostgreSQL (например, 12, 13, 14, 15, 16).

```bash
# Открыть файл конфигурации аутентификации (замените [версия] на вашу версию PostgreSQL)
sudo nano /etc/postgresql/[версия]/main/pg_hba.conf
```

Найти строку:
```
# "local" is for Unix domain socket connections only
local   all             all                                     peer
```

Изменить `peer` на `md5`:
```
# "local" is for Unix domain socket connections only
local   all             all                                     md5
```

### 10. Перезапустить PostgreSQL
```bash
sudo systemctl restart postgresql
```

Теперь PostgreSQL будет требовать пароль для локальных подключений вместо peer аутентификации.

## Настройка временной зоны

### Настройка временной зоны в PostgreSQL
```sql
-- Установить московскую временную зону для сессии
SET timezone = 'Europe/Moscow';
```

### Глобальная настройка временной зоны системы
```bash
# Установить московскую временную зону на уровне ОС
sudo timedatectl set-timezone Europe/Moscow

# Проверить настройку
timedatectl

# Перезапустить PostgreSQL для применения новой зоны
sudo systemctl restart postgresql

# Перезапустить Flask API (если используется systemd)
sudo systemctl restart lora-api.service
