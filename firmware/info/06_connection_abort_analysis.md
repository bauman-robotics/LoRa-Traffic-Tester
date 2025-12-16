# Анализ проблемы "Software caused connection abort" (errno 113)

## Описание проблемы

После исправления проблемы с памятью появилась новая ошибка сетевого подключения:

```
[162303][E][ssl_client.cpp:129] start_ssl_client(): socket error on fd 48, errno: 113, "Software caused connection abort"
[162315][E][WiFiClientSecure.cpp:144] connect(): start_ssl_client: -1
[162321][E][wifi_manager.cpp:579] doHttpPostFromData(): [WiFiManager] Cannot connect to server 84.252.143.212
```

**Errno 113 = ECONNABORTED** - программное прерывание соединения со стороны сервера.

## Анализ причины

### Ключевые наблюдения
- **Ошибка возникает сразу при подключении** - до отправки данных
- **Failed Packets счетчик растет**: 1 → 2
- **Response Time остается 0** - нет успешных соединений
- **LoRa пакеты принимаются нормально** - устройство работает
- **Heap памяти достаточно**: 162572 bytes free

### Возможные причины

#### 1. **Серверные ограничения**
- **Rate limiting**: Слишком частые запросы от одного IP
- **Connection limits**: Сервер ограничивает количество одновременных соединений
- **Geographic blocking**: Сервер блокирует запросы из определенных регионов
- **Resource exhaustion**: Сервер перегружен

#### 2. **Сетевые проблемы**
- **NAT timeout**: Роутер закрывает idle TCP соединения через 5-30 минут
- **ISP blocking**: Провайдер блокирует HTTPS трафик
- **Firewall/IDS**: Система обнаружения вторжений блокирует "подозрительную" активность
- **DNS issues**: Проблемы с разрешением домена

#### 3. **Проблемы SSL/TLS**
- **SSL handshake failure**: Несмотря на insecure mode
- **Certificate issues**: Проблемы с цепочкой сертификатов
- **Protocol mismatch**: Несовместимость версий TLS

#### 4. **Проблемы с данными**
- **Malformed requests**: Некорректный JSON или HTTP заголовки
- **Content validation**: Сервер отклоняет данные

## Диагностика проблемы

### 1. Тестирование сервера
```bash
# Прямой тест с curl (имитация ESP32)
curl -v -k -X POST \
  -H 'Content-Type: application/json' \
  -H 'User-Agent: curl/7.81.0' \
  -d '{"user_id":"Guest","user_location":"Home","sender_nodeid":"69842E50","destination_nodeid":"FFFFFFFF","full_packet_len":58,"signal_level_dbm":1}' \
  https://84.252.143.212/api/lora
```

### 2. Проверка сетевых настроек
```bash
# Traceroute для проверки пути
traceroute 84.252.143.212

# Ping для проверки доступности
ping 84.252.143.212

# Проверка DNS
nslookup 84.252.143.212
```

### 3. Тестирование разных протоколов
```bash
# HTTP вместо HTTPS
curl -v -X POST \
  -H 'Content-Type: application/json' \
  -d '{"test": "data"}' \
  http://84.252.143.212/api/lora

# Разные User-Agent
curl -v -k -X POST \
  -H 'User-Agent: ESP32' \
  -d '{"test": "data"}' \
  https://84.252.143.212/api/lora
```

## Предлагаемые решения

### Решение 1: Задержка между запросами
```cpp
// Добавить в doHttpPostFromData() перед отправкой
static unsigned long lastRequestTime = 0;
unsigned long currentTime = millis();
unsigned long timeSinceLastRequest = currentTime - lastRequestTime;

if (timeSinceLastRequest < 5000) { // Минимум 5 секунд между запросами
    ESP_LOGW(TAG, "Request too soon, delaying...");
    vTaskDelay(pdMS_TO_TICKS(5000 - timeSinceLastRequest));
}

lastRequestTime = millis();
```

### Решение 2: Переход на HTTP для тестирования
```cpp
// Временно в lora_config.hpp
#define USE_HTTPS 0  // Отключить HTTPS
```

### Решение 3: Exponential backoff
```cpp
// Добавить retry логику с увеличением задержки
int retryCount = 0;
const int MAX_RETRIES = 3;

while (retryCount < MAX_RETRIES) {
    if (client.connect(serverIP.c_str(), port)) {
        // Успешное подключение
        break;
    }
    retryCount++;
    unsigned long delayMs = (1 << retryCount) * 1000; // 2s, 4s, 8s
    ESP_LOGW(TAG, "Connection failed, retry %d in %lu ms", retryCount, delayMs);
    vTaskDelay(pdMS_TO_TICKS(delayMs));
}
```

### Решение 4: Connection pooling
```cpp
// Переиспользование SSL клиента
static WiFiClientSecure* persistentClient = nullptr;

if (!persistentClient) {
    persistentClient = new WiFiClientSecure();
    persistentClient->setInsecure();
}

if (!persistentClient->connected()) {
    if (!persistentClient->connect(serverIP.c_str(), port)) {
        ESP_LOGE(TAG, "Failed to connect with persistent client");
        return;
    }
}

// Использовать persistentClient
```

### Решение 5: Диагностический режим
```cpp
// Добавить флаг для подробной диагностики
#define CONNECTION_DEBUG 1

#if CONNECTION_DEBUG
ESP_LOGI(TAG, "Connection attempt details:");
ESP_LOGI(TAG, "  Server: %s:%d", serverIP.c_str(), port);
ESP_LOGI(TAG, "  WiFi status: %d", WiFi.status());
ESP_LOGI(TAG, "  Local IP: %s", WiFi.localIP().toString().c_str());
ESP_LOGI(TAG, "  Gateway: %s", WiFi.gatewayIP().toString().c_str());
#endif
```

### Решение 6: Альтернативные endpoints
```cpp
// Попробовать разные URL
const char* alternativeUrls[] = {
    "https://84-252-143-212.nip.io:443/api/lora",
    "http://84.252.143.212:80/api/lora",
    "https://httpbin.org/post"  // Тестовый endpoint
};

for (auto url : alternativeUrls) {
    ESP_LOGI(TAG, "Trying alternative URL: %s", url);
    // Попытка подключения к альтернативному URL
}
```

## План действий

### Шаг 1: Быстрая диагностика
1. **Протестировать сервер** с curl с того же IP
2. **Проверить логи сервера** на предмет блокировки
3. **Добавить задержку 5-10 секунд** между запросами

### Шаг 2: Временные обходные решения
1. **Переключиться на HTTP** для тестирования
2. **Увеличить задержки** между запросами
3. **Добавить connection pooling**

### Шаг 3: Долгосрочные улучшения
1. **Реализовать exponential backoff**
2. **Добавить health checks** сервера
3. **Мониторить connection success rate**
4. **Рассмотреть MQTT** как альтернативу HTTP

## Мониторинг и статистика

### Добавить счетчики для диагностики
```cpp
// В wifi_manager.hpp
volatile unsigned long connectionAttempts = 0;
volatile unsigned long connectionFailures = 0;
volatile unsigned long serverRejections = 0;

// В коде
connectionAttempts++;
if (client.connect(...) == false) {
    connectionFailures++;
    // Определить тип ошибки
    if (error.contains("abort")) {
        serverRejections++;
    }
}
```

### Логи статистики
```
Connection Stats:
- Total attempts: 150
- Success rate: 95%
- Server rejections: 5
- Network failures: 3
```

## Заключение

Проблема "Software caused connection abort" указывает на то, что сервер активно отклоняет соединения ESP32. Это может быть вызвано:

1. **Rate limiting** на сервере
2. **Security policies** (firewall/IDS)
3. **Resource limits** сервера
4. **Network configuration** issues

Рекомендуется начать с добавления задержек между запросами и тестирования сервера с curl. Если проблема persists, нужно проверять server-side логи и настройки.
