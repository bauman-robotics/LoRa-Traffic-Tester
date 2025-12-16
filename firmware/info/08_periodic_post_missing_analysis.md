# Анализ проблемы отсутствия периодических POST запросов

## Описание проблемы

Устройство работает в режиме **TRANSMITTER** с конфигурацией:
- `POST_EN_WHEN_LORA_RECEIVED = 0` (отправка при получении LoRa отключена)
- `POST_INTERVAL_EN = 1` (периодическая отправка POST включена)

**Ожидаемое поведение:**
- POST запросы должны отправляться каждые `POST_INTERVAL_MS` миллисекунд
- В логах должны быть блоки `================== POST REQUEST ==================`

**Фактическое поведение:**
- POST запросы не отправляются вообще
- Нет логов о периодических POST попытках
- Нет диагностических блоков POST REQUEST

## Анализ возможных причин

### 1. **POST Task не запускается**
```cpp
// В wifi_manager.cpp enable()
// POST task должен запускаться здесь:
if (connect()) {
    lastHttpResult = "WiFi connected, waiting for first POST...";
    startPOSTTask();  // <-- Проверить, вызывается ли это
}
```

### 2. **POST_INTERVAL_EN не влияет на логику**
```cpp
// В wifi_manager.cpp httpPostTask()
// Периодические POST контролируются post_on_lora, а не POST_INTERVAL_EN
if (!post_on_lora) {  // post_on_lora = POST_EN_WHEN_LORA_RECEIVED = 0
    // Это условие выполняется, POST должны отправляться
    if (enabled && isConnected() && postEnabled) {
        doHttpPost();
    }
    vTaskDelay(pdMS_TO_TICKS(POST_INTERVAL_MS));
}
```

### 3. **postEnabled не установлен**
```cpp
// postEnabled должен быть установлен в true
postEnabled = POST_INTERVAL_EN;  // POST_INTERVAL_EN = 1
```

### 4. **POST_INTERVAL_MS слишком большой**
```cpp
#define POST_INTERVAL_MS 60000  // 60 секунд - может быть слишком долго для тестирования
```

### 5. **WiFiManager не включен**
```cpp
// В control.cpp setup()
if (WIFI_ENABLE) {
    m_wifiManager->enable(true);  // <-- Проверить выполнение
}
```

## Диагностические улучшения

### 1. Добавить логи в POST task
```cpp
// В wifi_manager.cpp httpPostTask()
ESP_LOGI(TAG, "=== POST TASK DEBUG ===");
ESP_LOGI(TAG, "post_on_lora=%d", post_on_lora);
ESP_LOGI(TAG, "enabled=%d", enabled);
ESP_LOGI(TAG, "isConnected()=%d", isConnected());
ESP_LOGI(TAG, "postEnabled=%d", postEnabled);
ESP_LOGI(TAG, "POST_INTERVAL_MS=%d", POST_INTERVAL_MS);

if (!post_on_lora) {
    ESP_LOGI(TAG, "POST task: periodic mode active");
    bool cond1 = enabled;
    bool cond2 = isConnected();
    bool cond3 = postEnabled;
    if (cond1 && cond2 && cond3) {
        ESP_LOGI(TAG, "POST task: all conditions met, sending periodic POST");
        doHttpPost();
    } else {
        ESP_LOGI(TAG, "POST task: conditions not met - enabled=%d, connected=%d, postEnabled=%d",
                 cond1, cond2, cond3);
    }
    ESP_LOGI(TAG, "POST task: sleeping for %d ms", POST_INTERVAL_MS);
    vTaskDelay(pdMS_TO_TICKS(POST_INTERVAL_MS));
} else {
    ESP_LOGI(TAG, "POST task: LoRa trigger mode active");
    vTaskDelay(pdMS_TO_TICKS(500));
}
ESP_LOGI(TAG, "=== POST TASK DEBUG END ===");
```

### 2. Проверить инициализацию WiFiManager
```cpp
// В control.cpp setup()
ESP_LOGI(TAG, "=== WIFI INIT DEBUG ===");
ESP_LOGI(TAG, "WIFI_ENABLE=%d", WIFI_ENABLE);
ESP_LOGI(TAG, "POST_INTERVAL_EN=%d", POST_INTERVAL_EN);
ESP_LOGI(TAG, "POST_EN_WHEN_LORA_RECEIVED=%d", POST_EN_WHEN_LORA_RECEIVED);

if (WIFI_ENABLE) {
    ESP_LOGI(TAG, "Enabling WiFiManager...");
    m_wifiManager->enable(true);
    ESP_LOGI(TAG, "WiFiManager enabled, postEnabled=%d", m_wifiManager->isPostEnabled());
} else {
    ESP_LOGI(TAG, "WiFi disabled by WIFI_ENABLE=0");
}
ESP_LOGI(TAG, "=== WIFI INIT DEBUG END ===");
```

### 3. Мониторить состояние POST task
```cpp
// Добавить в wifi_manager.hpp
volatile bool postTaskRunning = false;

// В startPOSTTask()
postTaskRunning = true;

// В httpPostTask() начале
ESP_LOGD(TAG, "POST task running, postTaskRunning=%d", postTaskRunning);
```

## Предлагаемые решения

### Решение 1: Уменьшить интервал для тестирования
```cpp
// Временно уменьшить интервал для отладки
#define POST_INTERVAL_MS 10000  // 10 секунд вместо 60
```

### Решение 2: Принудительно отправить POST
```cpp
// Добавить в serial команды
} else if (c_cmp(cmd_token, "send_post")) {
    ESP_LOGI(TAG, "Manual POST trigger");
    m_wifiManager->sendSinglePost();
    return;  // Handled
```

### Решение 3: Проверить POST task создание
```cpp
// В startPOSTTask()
if (httpTaskHandle == nullptr) {
    ESP_LOGI(TAG, "Creating POST task...");
    xTaskCreate(httpPostTaskWrapper, "HTTPTask", 32768, this, 1, &httpTaskHandle);
    if (httpTaskHandle != nullptr) {
        ESP_LOGI(TAG, "POST task created successfully");
    } else {
        ESP_LOGE(TAG, "POST task creation failed!");
    }
} else {
    ESP_LOGW(TAG, "POST task already exists");
}
```

### Решение 4: Добавить watchdog для POST task
```cpp
// В httpPostTask() добавить счетчик
static unsigned long lastPostTime = 0;
unsigned long currentTime = millis();

if (currentTime - lastPostTime > POST_INTERVAL_MS * 2) {
    ESP_LOGW(TAG, "POST task may be stuck, last post %lu ms ago", currentTime - lastPostTime);
}

if (sent) {
    lastPostTime = currentTime;
}
```

## План действий

### Шаг 1: Добавить диагностику
1. **Добавить логи в POST task** для отслеживания выполнения
2. **Уменьшить POST_INTERVAL_MS** для быстрого тестирования
3. **Добавить команду ручной отправки POST**

### Шаг 2: Тестирование
1. **Пересобрать** с диагностикой
2. **Подождать POST_INTERVAL_MS** и проверить логи
3. **Отправить ручной POST** через serial

### Шаг 3: Анализ результатов
1. **Если POST task не запускается** - проблема в инициализации
2. **Если условия не выполняются** - проблема в состоянии WiFi/postEnabled
3. **Если task зависает** - проблема в doHttpPost()

## Мониторинг

### Ожидаемые логи после исправления:
```
=== POST TASK DEBUG ===
post_on_lora=0
enabled=1
isConnected()=1
postEnabled=1
POST_INTERVAL_MS=10000
POST task: periodic mode active
POST task: all conditions met, sending periodic POST
=== POST TASK DEBUG END ===

================== POST REQUEST ==================
Protocol: HTTPS
...
================== POST END ==================
```

### Метрики для отслеживания:
```
POST Statistics:
- Task running: YES/NO
- Conditions met: YES/NO
- Last POST: X seconds ago
- Interval: X seconds
```

## Заключение

Проблема отсутствия периодических POST запросов может быть вызвана:
1. **POST task не запускается** при инициализации WiFiManager
2. **Условия отправки не выполняются** (WiFi отключен или postEnabled=false)
3. **POST_INTERVAL_MS слишком большой** для тестирования
4. **POST task зависает** в doHttpPost()

Диагностические логи помогут точно определить причину и выбрать правильное решение.
