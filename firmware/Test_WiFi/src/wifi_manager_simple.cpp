#include "wifi_manager_simple.hpp"

WiFiManagerSimple::WiFiManagerSimple() :
    hot_value(POST_HOT_VALUE),
    cold_value(POST_COLD_VALUE),
    postsSent(0),
    lastPostTime(0),
    connectionTime(0),
    lastSuccessfulPost(0),
    isCurrentlyConnected(false)
{
    // Load network settings
    ssid = DEFAULT_WIFI_SSID;
    password = DEFAULT_WIFI_PASSWORD;
    apiKey = DEFAULT_API_KEY;
    userId = DEFAULT_USER_ID;
    userLocation = DEFAULT_USER_LOCATION;
    serverProtocol = DEFAULT_SERVER_PROTOCOL;
    serverIP = DEFAULT_SERVER_IP;
    serverPath = DEFAULT_SERVER_PATH;
    serverPort = DEFAULT_SERVER_PORT;

    ESP_LOGI(TAG, "WiFi Test Manager initialized");
    ESP_LOGI(TAG, "Server: %s://%s/%s", serverProtocol.c_str(), serverIP.c_str(), serverPath.c_str());
    ESP_LOGI(TAG, "POST values: hot=%d, cold=%d", hot_value, cold_value);
    ESP_LOGI(TAG, "POST interval: %d ms", POST_INTERVAL_MS);
}

WiFiManagerSimple::~WiFiManagerSimple() {
    disconnect();
}

bool WiFiManagerSimple::connect() {
#if WIFI_DEBUG_FIXES
    WiFi.mode(WIFI_STA);
    WiFi.setSleep(false);
    WiFi.setAutoReconnect(true);
    WiFi.setAutoConnect(true);
    // WiFi.setPersistent not available in Arduino ESP32
    WiFi.onEvent(WiFiEvent);
    ESP_LOGI(TAG, "WiFi debug fixes enabled");
#endif

    ESP_LOGI(TAG, "Connecting to WiFi: %s", ssid.c_str());

#if WIFI_AUTO_TX_POWER_TEST
    // Auto TX power test (simplified)
    wifi_power_t txPowers[] = {WIFI_POWER_8_5dBm, WIFI_POWER_18dBm, WIFI_POWER_15dBm};
    for (size_t v = 0; v < sizeof(txPowers)/sizeof(wifi_power_t); ++v) {
        WiFi.setTxPower(static_cast<wifi_power_t>(txPowers[v]));
        ESP_LOGI(TAG, "Trying TX power variant %d", (int)v);
        WiFi.begin(ssid.c_str(), password.c_str());

        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
            delay(WIFI_CONNECT_INTERVAL_MS);
            ESP_LOGD(TAG, "Connecting... Status: %d", (int)WiFi.status());
        }

        if (WiFi.status() == WL_CONNECTED) {
            ESP_LOGI(TAG, "WiFi connected! IP: %s", WiFi.localIP().toString().c_str());
            connectionTime = millis();
            isCurrentlyConnected = true;
            return true;
        }
    }
#else
    WiFi.mode(WIFI_STA);
    WiFi.setTxPower(static_cast<wifi_power_t>(WIFI_TX_POWER));
    ESP_LOGI(TAG, "TX power set before begin: %d", WIFI_TX_POWER);

    WiFi.begin(ssid.c_str(), password.c_str());

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && (millis() - start) < WIFI_CONNECT_TIMEOUT_MS) {
        delay(WIFI_CONNECT_INTERVAL_MS);
        ESP_LOGD(TAG, "Connecting... Status: %d", (int)WiFi.status());
    }

    if (WiFi.status() == WL_CONNECTED) {
        ESP_LOGI(TAG, "WiFi connected! IP: %s", WiFi.localIP().toString().c_str());
        connectionTime = millis();
        isCurrentlyConnected = true;
        return true;
    }
#endif

    ESP_LOGE(TAG, "WiFi connection failed");
    isCurrentlyConnected = false;
    return false;
}

void WiFiManagerSimple::disconnect() {
    WiFi.disconnect();
    isCurrentlyConnected = false;
    ESP_LOGI(TAG, "WiFi disconnected");
}

bool WiFiManagerSimple::isConnected() {
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiManagerSimple::doHttpPost() {
    if (!isConnected()) {
        ESP_LOGW(TAG, "Not connected to WiFi, skipping POST");
        return false;
    }

    WiFiClient client;
    int port = serverPort.toInt();
    String path = "/api/lora";

    // Increment cold counter (simple implementation)
    cold_value++;

    String alarm_time = String(ALARM_TIME + random(0, 1000));

    // JSON format
    String postData = "{";
    postData += "\"api_key\":\"" + apiKey + "\",";
    postData += "\"user_id\":\"" + userId + "\",";
    postData += "\"user_location\":\"" + userLocation + "\",";
    postData += "\"cold\":" + String(cold_value) + ",";
    postData += "\"hot\":" + String(hot_value) + ",";
    postData += "\"alarm_time\":\"" + alarm_time + "\"";
    postData += "}";

    ESP_LOGI(TAG, "POST #%lu: cold=%d, hot=%d", postsSent + 1, cold_value, hot_value);
    ESP_LOGI(TAG, "POST to: http://%s:%d%s", serverIP.c_str(), port, path.c_str());
    ESP_LOGI(TAG, "Curl: curl -X POST http://%s:%d%s -H \"Content-Type: application/json\" -d '%s'",
             serverIP.c_str(), port, path.c_str(), postData.c_str());

    if (client.connect(serverIP.c_str(), port)) {
        client.setTimeout(5000);
        ESP_LOGD(TAG, "Connected to server");

        client.println("POST " + path + " HTTP/1.1");
        client.println("Host: " + serverIP);
        client.println("User-Agent: WiFi-Test-Device");
        client.println("Content-Type: application/json");
        client.println("Content-Length: " + String(postData.length()));
        client.println("Connection: close");
        client.println();
        client.println(postData);

        ESP_LOGD(TAG, "POST sent, waiting for response...");

        // Wait for response
        unsigned long startTime = millis();
        String response = "";

        while (client.connected() && (millis() - startTime) < 5000) {
            if (client.available()) {
                char c = client.read();
                response += c;
                if (response.endsWith("\r\n\r\n")) break; // Headers received
            }
            delay(10);
        }

        client.stop();

        if (response.indexOf("200") >= 0 || response.length() > 0) {
            lastSuccessfulPost = millis();
            postsSent++;
            ESP_LOGI(TAG, "POST #%lu successful (cold=%d)", postsSent, cold_value);
            return true;
        } else {
            ESP_LOGE(TAG, "POST failed: response len=%d", response.length());
            return false;
        }
    } else {
        ESP_LOGE(TAG, "Cannot connect to server %s:%d", serverIP.c_str(), port);
        return false;
    }
}

#if WIFI_DEBUG_FIXES
void WiFiManagerSimple::WiFiEvent(WiFiEvent_t event) {
    switch (event) {
        case SYSTEM_EVENT_STA_DISCONNECTED:
            ESP_LOGW(TAG, "WiFi lost connection - attempting reconnect...");
            WiFi.reconnect();
            break;
        default:
            break;
    }
}
#endif
