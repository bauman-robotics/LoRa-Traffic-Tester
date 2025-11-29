#include "wifi_manager.hpp"
#include <string>
#include "LittleFS.h"
#if USE_HTTPS
#include <WiFiClientSecure.h>
#endif

// LoRa packet payload length storage
int lastLoRaPacketLen = 0;

WiFiManager::WiFiManager() {
  // Initialize WiFi mode and other setup
  WiFi.mode(WIFI_STA);
}

void WiFiManager::setLastLoRaPacketLen(int len) {
  lastLoRaPacketLen = len;
}

int WiFiManager::getLastLoRaPacketLen() {
  return lastLoRaPacketLen;
}

void WiFiManager::loadSettings() {
  // Try to load WiFi credentials from flash first, fall back to defaults
  loadWiFiCredentials();

  // Load server settings
#if USE_FLASK_SERVER
  // Use Flask server settings
  apiKey = DEFAULT_FLASK_API_KEY;
  userId = DEFAULT_FLASK_USER_ID;
  userLocation = DEFAULT_FLASK_USER_LOCATION;
  serverProtocol = DEFAULT_FLASK_SERVER_PROTOCOL;
  serverIP = DEFAULT_FLASK_SERVER_IP;
  serverPort = DEFAULT_FLASK_SERVER_PORT;
  serverPath = DEFAULT_FLASK_SERVER_PATH;
#else
  // Use PHP server settings (default)
  apiKey = DEFAULT_API_KEY;
  userId = DEFAULT_USER_ID;
  userLocation = DEFAULT_USER_LOCATION;
  serverProtocol = DEFAULT_SERVER_PROTOCOL;
  serverIP = DEFAULT_SERVER_IP;
#ifdef USE_HTTPS
  serverPort = "443";  // Default HTTPS port for PHP server
#else
  serverPort = "80";  // Default HTTP port for PHP server
#endif
  serverPath = DEFAULT_SERVER_PATH;
#endif
  ESP_LOGI(TAG, "Settings loaded from defaults");
  std::string settings = "Network defaults:\n";
  settings += "SSID: " + std::string(ssid.c_str()) + "\n";
  settings += "Password: ****\n";
  settings += "API key: " + std::string(DEFAULT_FLASK_API_KEY) + "\n";
  settings += "User ID: " + std::string(DEFAULT_FLASK_USER_ID) + "\n";
  settings += "User location: " + std::string(DEFAULT_FLASK_USER_LOCATION) + "\n";
  settings += "Server protocol: " + std::string(DEFAULT_SERVER_PROTOCOL) + "\n";
  settings += "Server IP: " + std::string(DEFAULT_FLASK_SERVER_IP) + "\n";
  settings += "Server path: " + std::string(DEFAULT_FLASK_SERVER_PATH);
  ESP_LOGI(TAG, "%s", settings.c_str());
  ESP_LOGI(TAG, "LoRa defines:");
  ESP_LOGI(TAG, "  FAKE_LORA=%d", FAKE_LORA);
  ESP_LOGI(TAG, "  LORA_STATUS_ENABLED=%d", LORA_STATUS_ENABLED);
  ESP_LOGI(TAG, "  LORA_STATUS_INTERVAL_SEC=%d", LORA_STATUS_INTERVAL_SEC);
  ESP_LOGI(TAG, "  LORA_STATUS_SHORT_PACKETS=%d", LORA_STATUS_SHORT_PACKETS);
  ESP_LOGI(TAG, "  POST_INTERVAL_EN=%d", POST_INTERVAL_EN);
  ESP_LOGI(TAG, "  POST_EN_WHEN_LORA_RECEIVED=%d", POST_EN_WHEN_LORA_RECEIVED);
  ESP_LOGI(TAG, "  POST_HOT_AS_RSSI=%d, POST_BATCH_ENABLED=%d", POST_HOT_AS_RSSI, POST_BATCH_ENABLED);
  ESP_LOGI(TAG, "MESH settings:");
  ESP_LOGI(TAG, "  MESH_COMPATIBLE=%d", MESH_COMPATIBLE);
  ESP_LOGI(TAG, "  MESH_SYNC_WORD=0x%02X", MESH_SYNC_WORD);
  ESP_LOGI(TAG, "  MESH_FREQUENCY=%.3f", MESH_FREQUENCY);
  ESP_LOGI(TAG, "  MESH_BANDWIDTH=%d", MESH_BANDWIDTH);
  ESP_LOGI(TAG, "  MESH_SPREADING_FACTOR=%d", MESH_SPREADING_FACTOR);
  ESP_LOGI(TAG, "  MESH_CODING_RATE=%d", MESH_CODING_RATE);
  ESP_LOGI(TAG, "  OLD_LORA_PARS=%d", OLD_LORA_PARS);
  ESP_LOGI(TAG, "  COLD_AS_LORA_PAYLOAD_LEN=%d", COLD_AS_LORA_PAYLOAD_LEN);
  ESP_LOGI(TAG, "HTTPS settings:");
  ESP_LOGI(TAG, "  USE_HTTPS=%d", USE_HTTPS);
  ESP_LOGI(TAG, "  USE_INSECURE_HTTPS=%d", USE_INSECURE_HTTPS);
}

void WiFiManager::saveWiFiCredentials() {
  // Save WiFi credentials to LittleFS
  if (!LittleFS.begin()) {
    ESP_LOGE(TAG, "Failed to initialize LittleFS for WiFi credentials save");
    return;
  }

  File wifiFile = LittleFS.open("/wifi_credentials.txt", FILE_WRITE);
  if (!wifiFile) {
    ESP_LOGE(TAG, "Failed to open WiFi credentials file for writing");
    return;
  }

  wifiFile.println(ssid);
  wifiFile.println(password);
  wifiFile.close();

  ESP_LOGI(TAG, "WiFi credentials saved to flash: SSID=%s", ssid.c_str());
}

void WiFiManager::loadWiFiCredentials() {
  // Load WiFi credentials from LittleFS, fall back to defaults if not found or invalid
  if (!LittleFS.begin()) {
    ESP_LOGW(TAG, "Failed to initialize LittleFS for WiFi credentials load, using defaults");
    ssid = DEFAULT_WIFI_SSID;
    password = DEFAULT_WIFI_PASSWORD;
    return;
  }

  File wifiFile = LittleFS.open("/wifi_credentials.txt", FILE_READ);
  if (!wifiFile) {
    ESP_LOGI(TAG, "WiFi credentials file not found, using defaults");
    ssid = DEFAULT_WIFI_SSID;
    password = DEFAULT_WIFI_PASSWORD;
    return;
  }

  String loaded_ssid = wifiFile.readStringUntil('\n');
  String loaded_password = wifiFile.readStringUntil('\n');
  wifiFile.close();

  // Trim whitespace
  loaded_ssid.trim();
  loaded_password.trim();

  // Validate loaded credentials - if valid, use them; otherwise use defaults
  if (loaded_ssid.length() > 0 && loaded_password.length() >= 8) {
    ssid = loaded_ssid;
    password = loaded_password;
    ESP_LOGI(TAG, "WiFi credentials loaded from flash: SSID=%s", ssid.c_str());
  } else {
    ESP_LOGW(TAG, "Invalid WiFi credentials in flash, using defaults");
    ssid = DEFAULT_WIFI_SSID;
    password = DEFAULT_WIFI_PASSWORD;
  }
}

void WiFiManager::setWiFiCredentials(const String& new_ssid, const String& new_password) {
  if (new_ssid.length() > 0 && new_password.length() >= 8) {
    ssid = new_ssid;
    password = new_password;
    saveWiFiCredentials();
    ESP_LOGI(TAG, "WiFi credentials updated: SSID=%s", ssid.c_str());
  } else {
    ESP_LOGE(TAG, "Invalid WiFi credentials provided");
  }
}

bool WiFiManager::connect() {
#if WIFI_ENABLE == 0
  ESP_LOGI(TAG, "WiFi disabled by WIFI_ENABLE=0");
  return false;
#endif
#if WIFI_DEBUG_FIXES && WIFI_ENABLE
  // Scan networks first
  ESP_LOGI(TAG, "Scanning WiFi networks...");
  int n = WiFi.scanNetworks();
  bool foundTarget = false;
  
  for (int i = 0; i < n; ++i) {
    String encryption;
    switch (WiFi.encryptionType(i)) {
      case WIFI_AUTH_OPEN: encryption = "Open"; break;
      case WIFI_AUTH_WEP: encryption = "WEP"; break;
      case WIFI_AUTH_WPA_PSK: encryption = "WPA"; break;
      case WIFI_AUTH_WPA2_PSK: encryption = "WPA2"; break;
      case WIFI_AUTH_WPA_WPA2_PSK: encryption = "WPA/WPA2"; break;
      case WIFI_AUTH_WPA2_ENTERPRISE: encryption = "WPA2-Enterprise"; break;
      case WIFI_AUTH_WPA3_PSK: encryption = "WPA3"; break;
      default: encryption = "Unknown";
    }

    // ИСПРАВЛЕННЫЙ КОД ДЛЯ ОПРЕДЕЛЕНИЯ ШИРИНЫ КАНАЛА
    String channelWidth = "20MHz";
    uint8_t primaryChannel = (uint8_t)WiFi.channel(i);
    wifi_second_chan_t secondChannel = WIFI_SECOND_CHAN_NONE;
    
    // Используем правильные типы
    esp_err_t result = esp_wifi_get_channel(&primaryChannel, &secondChannel);
    
    if (result == ESP_OK) {
      if (secondChannel == WIFI_SECOND_CHAN_ABOVE) {
        channelWidth = "40MHz (above)";
      } else if (secondChannel == WIFI_SECOND_CHAN_BELOW) {
        channelWidth = "40MHz (below)";
      } else {
        channelWidth = "20MHz";
      }
    } else {
      channelWidth = "Unknown";
    }

    // ВЫВОДИМ ШИРИНУ КАНАЛА В ЛОГ
    ESP_LOGI(TAG, "Network: %-20s RSSI: %3ddBm Ch: %2d Width: %-12s Auth: %-15s",
             WiFi.SSID(i).c_str(), WiFi.RSSI(i), WiFi.channel(i), 
             channelWidth.c_str(), encryption.c_str());

    if (WiFi.SSID(i) == ssid) {
      foundTarget = true;
      ESP_LOGI(TAG, "*** TARGET FOUND: %s - Channel %d, %s", 
               ssid.c_str(), WiFi.channel(i), channelWidth.c_str());
    }
  }
  
  if (!foundTarget) {
    ESP_LOGW(TAG, "TARGET NETWORK %s NOT FOUND in %d scanned networks!", ssid.c_str(), n);
  }
#endif

  ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", ssid.c_str());
  ESP_LOGI(TAG, "WiFi password: ****");
  ESP_LOGI(TAG, "API key: %s", apiKey.c_str());
  String fullPath = serverPath.startsWith("/") ? serverPath : "/" + serverPath;
  ESP_LOGI(TAG, "Server: %s%s", serverIP.c_str(), fullPath.c_str());
#if USE_HTTPS
  String nipIoUrl = getNipIoUrl(serverIP, serverPort.toInt(), fullPath);
  ESP_LOGI(TAG, "Full server URL: %s", nipIoUrl.c_str());
#else
  ESP_LOGI(TAG, "Full server URL: %s://%s%s", serverProtocol.c_str(), serverIP.c_str(), fullPath.c_str());
#endif
  uint8_t mac[6];
  WiFi.macAddress(mac);
  char macStr[18];
  sprintf(macStr, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  ESP_LOGI(TAG, "MAC: %s", macStr);

#if WIFI_DEBUG_FIXES
  WiFi.setSleep(false); // Отключить спящий режим WiFi
  WiFi.setAutoReconnect(true); // Автопереподключение
  WiFi.persistent(true); // Сохранять настройки WiFi
  WiFi.onEvent(WiFiEvent);
  ESP_LOGI(TAG, "WiFi debug fixes enabled: sleep off, autoreconnect on, persistent on, event callback set");
#endif

#if WIFI_AUTO_TX_POWER_TEST
  WiFi.mode(WIFI_STA);  // Always set mode before setTxPower
  // Automatic TX power testing
  wifi_power_t txPowers[] = {static_cast<wifi_power_t>(44), // 20dBm
                             static_cast<wifi_power_t>(43), // 19.5dBm
                             static_cast<wifi_power_t>(42), // 19dBm
                             static_cast<wifi_power_t>(41), // 18.5dBm
                             static_cast<wifi_power_t>(40), // 18dBm
                             static_cast<wifi_power_t>(34), // 15dBm
                             static_cast<wifi_power_t>(26), // 11dBm
                             static_cast<wifi_power_t>(21), // 8.5dBm
                             static_cast<wifi_power_t>(8)}; // 2dBm
  for (size_t v = 0; v < sizeof(txPowers)/sizeof(wifi_power_t); ++v) {
    ESP_LOGI(TAG, "Trying TX power variant %d (enum val %d = %0.1f dBm)", (int)v, txPowers[v], (float)(txPowers[v] - 8) / 2.0f);
    WiFi.setTxPower(txPowers[v]);  // Set TX power BEFORE begin for AUTH_EXPIRE fix
    ESP_LOGI(TAG, "WiFi TX power set before begin: %d", txPowers[v]);
    WiFi.begin(ssid.c_str(), password.c_str());
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < WIFI_ATTEMPTS_PER_VARIANT) {
      ESP_LOGI(TAG, "Attempt %d/%d: status=%d", attempts + 1, WIFI_ATTEMPTS_PER_VARIANT, (int)WiFi.status());
      vTaskDelay(pdMS_TO_TICKS(WIFI_CONNECT_INTERVAL_MS));
      attempts++;
    }
    if (WiFi.status() == WL_CONNECTED) {
      ESP_LOGI(TAG, "WiFi connected with TX power variant %d (%d): IP %s", (int)v, txPowers[v], WiFi.localIP().toString().c_str());
      return true;
    } else {
      ESP_LOGW(TAG, "TX power variant %d (%d) failed after %d attempts, trying next", (int)v, txPowers[v], WIFI_ATTEMPTS_PER_VARIANT);
      WiFi.disconnect();  // Disconnect before next variant
      vTaskDelay(pdMS_TO_TICKS(1000));  // Brief delay before next variant
    }
  }
  ESP_LOGE(TAG, "All TX power variants failed");
  return false;
#else
  WiFi.mode(WIFI_STA);  // Always set mode before setTxPower
  WiFi.setTxPower((wifi_power_t)WIFI_TX_POWER);  // Set TX power BEFORE begin for AUTH_EXPIRE fix
  ESP_LOGI(TAG, "WiFi TX power set before begin: %d", WIFI_TX_POWER);
  WiFi.begin(ssid.c_str(), password.c_str());
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < WIFI_CONNECT_ATTEMPTS) {
    ESP_LOGI(TAG, "Attempt %d/%d: status=%d", attempts + 1, WIFI_CONNECT_ATTEMPTS, (int)WiFi.status());
    vTaskDelay(pdMS_TO_TICKS(WIFI_CONNECT_INTERVAL_MS));
    attempts++;
  }
  if (WiFi.status() == WL_CONNECTED) {
    ESP_LOGI(TAG, "WiFi connected: IP %s", WiFi.localIP().toString().c_str());
    ESP_LOGI(TAG, "Server protocol: %s, Port: %s", serverProtocol.c_str(), serverPort.c_str());
    WiFi.setTxPower((wifi_power_t)WIFI_TX_POWER);
    ESP_LOGI(TAG, "WiFi max TX power set to: %d (%0.1f dBm)", WIFI_TX_POWER, (float)(WIFI_TX_POWER - 8) / 2.0f);
    return true;
  } else {
    ESP_LOGE(TAG, "WiFi connection failed after %d attempts, final status=%d", WIFI_CONNECT_ATTEMPTS, (int)WiFi.status());
    return false;
  }
#endif
}

void WiFiManager::disconnect() {
  WiFi.disconnect();
  ESP_LOGI(TAG, "WiFi disconnected");
}

bool WiFiManager::isConnected() {
  return WiFi.status() == WL_CONNECTED;
}

void WiFiManager::enable(bool state) {
  if (state) {
    enabled = true;
    if (connect()) {
      lastHttpResult = "WiFi connected, waiting for first POST...";
      startPOSTTask();
      #if SERVER_PING_ENABLED
        startPingTask();
      #endif
      ESP_LOGI(TAG, "WiFi enabled");
    } else {
      enabled = false;
      disconnect();
      lastHttpResult = "WiFi connection failed";
      ESP_LOGI(TAG, "WiFi failed to connect, disabled");
    }
  } else {
    enabled = false;
    stopPOSTTask();
    #if SERVER_PING_ENABLED
      stopPingTask();
    #endif
    disconnect();
    lastHttpResult = "WiFi disabled";
    ESP_LOGI(TAG, "WiFi disabled");
  }
}

void WiFiManager::enablePost(bool state) {
  postEnabled = state;
  ESP_LOGI(TAG, "POST requests set to %s", state ? "ENABLED" : "DISABLED");
}

void WiFiManager::setPostOnLora(bool value) {
  if (post_on_lora_mm != value) {
    bool wasRunning = (httpTaskHandle != nullptr);
    if (wasRunning) {
      stopPOSTTask();
    }
    post_on_lora_mm = value;
    if (wasRunning) {
      startPOSTTask();
    }
    ESP_LOGI(TAG, "POST mode changed to: %s", value ? "LoRa trigger" : "Periodic");
  }
}

void WiFiManager::sendSinglePost() {
  if (enabled && isConnected()) {
    doHttpPost();
  }
}

void WiFiManager::httpPostTaskWrapper(void *param) {
  static_cast<WiFiManager*>(param)->httpPostTask();
}

void WiFiManager::startPOSTTask() {
  if (httpTaskHandle == nullptr) {
    xTaskCreate(httpPostTaskWrapper, "HTTPTask", 32768, this, 1, &httpTaskHandle);
  }
}

void WiFiManager::stopPOSTTask() {
  if (httpTaskHandle != nullptr) {
    vTaskDelete(httpTaskHandle);
    httpTaskHandle = nullptr;
  }
}

void WiFiManager::doHttpPostFromData(const String& postData) {
#if USE_HTTPS
  WiFiClientSecure client;
  #if USE_INSECURE_HTTPS
  client.setInsecure(); // Skip certificate verification (equivalent to curl -k)
  #endif
#else
  WiFiClient client;
#endif
  int port = serverPort.toInt();  // Use configurable port
  String path = serverPath.startsWith("/") ? serverPath : "/" + serverPath;
  String contentType = USE_FLASK_SERVER ? "application/json" : "application/x-www-form-urlencoded";

  ESP_LOGI(TAG, "Sending queued POST request");
  ESP_LOGI(TAG, "Full postData: %s", postData.c_str());

  // Log curl command for testing
  if (USE_FLASK_SERVER) {
    ESP_LOGI(TAG, "CURL test command: curl -k -X POST -H 'Content-Type: application/json' -d '%s' https://%s:%d%s",
             postData.c_str(), serverIP.c_str(), port, serverPath.c_str());
  } else {
    ESP_LOGI(TAG, "CURL test command: curl -k -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d '%s' https://%s:%d%s",
             postData.c_str(), serverIP.c_str(), port, serverPath.c_str());
  }

  lastHttpResult = "Sending queued POST request...";

  if (WiFi.status() != WL_CONNECTED) {
    ESP_LOGE(TAG, "WiFi not connected, cannot send POST");
    lastHttpResult = "WiFi not connected";
    return;
  }

  ESP_LOGI(TAG, "Connecting to server %s on port %d", serverIP.c_str(), port);

  // Log the full HTTP request being sent
  String fullRequest = "POST " + path + " HTTP/1.1\r\n";
  fullRequest += "Host: " + serverIP + "\r\n";
  fullRequest += "User-Agent: curl/7.81.0\r\n";
  fullRequest += "Content-Type: " + contentType + "\r\n";
  fullRequest += "Content-Length: " + String(postData.length()) + "\r\n";
  fullRequest += "Connection: close\r\n";
  fullRequest += "\r\n";
  fullRequest += postData;
  ESP_LOGI(TAG, "Full HTTP request being sent:\n%s", fullRequest.c_str());

  if (client.connect(serverIP.c_str(), port)) {
    client.setTimeout(5000); // Set timeout 5 seconds
    ESP_LOGI(TAG, "Connected to server for POST");
    ESP_LOGI(TAG, "Sending POST headers");
    client.println("POST " + path + " HTTP/1.1");
    client.println("Host: " + serverIP);
    client.println("User-Agent: curl/7.81.0");
    client.println("Content-Type: " + contentType);
    client.println("Content-Length: " + String(postData.length()));
    client.println("Connection: close");
    client.println();
    ESP_LOGI(TAG, "Sending POST data");
    client.println(postData);
    ESP_LOGI(TAG, "POST data sent, client connected: %d", client.connected());

    // Wait for response
    ESP_LOGI(TAG, "Waiting for server response...");
    delay(1000);  // Wait 1 second for response to start
    String response = "";
    unsigned long start = millis();
    bool responseStarted = false;
    while (client.connected() && (millis() - start < 8000)) {  // Increased timeout to 8 seconds
      if (client.available()) {
        if (!responseStarted) {
          ESP_LOGI(TAG, "Server response started");
          responseStarted = true;
        }
        int len = client.available();
        for (int i = 0; i < len; i++) {
          char c = client.read();
          response += c;
          if (response.endsWith("\r\n\r\n")) {
            ESP_LOGI(TAG, "HTTP headers received, response length: %d", response.length());
            break; // End of HTTP headers
          }
        }
        start = millis(); // Reset timeout if data received
      }
      delay(10); // Yield
    }

    if (!responseStarted) {
      ESP_LOGW(TAG, "No response received from server within timeout");
    }

    ESP_LOGI(TAG, "Response received, total length: %d", response.length());

    if (response.length() == 0) {
      ESP_LOGE(TAG, "No response data, possible server timeout or rejection");
    }
    if (response.length() > 0) {
      ESP_LOGI(TAG, "Response content: %s", response.substring(0, 100).c_str());
    }
    client.stop();

    if (response.indexOf("200") >= 0) {
      lastHttpResult = "Success: HTTP 200 (Queued POST sent)";
      ESP_LOGI(TAG, "Queued POST success");
    } else {
      lastHttpResult = "Failed: Server error";
      ESP_LOGE(TAG, "Queued POST failed: response len=%d", response.length());
    }
  } else {
    lastHttpResult = "Failed: Cannot connect to server " + serverIP;
    ESP_LOGE(TAG, "Cannot connect to server %s", serverIP.c_str());
  }
}

void WiFiManager::doHttpPost() {
#if USE_HTTPS
  WiFiClientSecure client;
  #if USE_INSECURE_HTTPS
  client.setInsecure(); // Skip certificate verification (equivalent to curl -k)
  #endif
#else
  WiFiClient client;
#endif
  extern unsigned long cold_counter;
  extern unsigned long hot_counter;

  int port = serverPort.toInt();  // Use configurable port
  String path = serverPath.startsWith("/") ? serverPath : "/" + serverPath;
  int alarm_value = ALARM_TIME + random(0, 10000);
  if (POST_SEND_SENDER_ID_AS_ALARM_TIME) {
    //alarm_value = last_sender_id & 0xFFFF;  // Use only lower 16 bits of sender ID, 0 if invalid header
    alarm_value = last_sender_id & 0xFFFF;
  }
  long hot_value;
  if (post_on_lora_mm) {
    hot_value = POST_HOT_AS_RSSI ? loraRssi : hot_counter;
  } else {
    hot_value = hot_counter;
  }
  long cold_value;
  if (post_on_lora_mm && (COLD_AS_LORA_PAYLOAD_LEN || !OLD_LORA_PARS)) {
    cold_value = getLastLoRaPacketLen();
    ESP_LOGD(TAG, "Using LoRa payload length as cold: %ld", cold_value);
  } else {
    cold_value = cold_counter++;
    ESP_LOGD(TAG, "Using cold counter: %ld", cold_value);
  }
  ESP_LOGD(TAG, "Alarm time: %d", alarm_value);

#if USE_FLASK_SERVER
  // Flask server - Enhanced JSON format with detailed LoRa packet info (HEX string format)
  String postData = "{";
  postData += "\"user_id\":\"" + userId + "\",";
  postData += "\"user_location\":\"" + userLocation + "\",";
  postData += "\"sender_nodeid\":\"" + uint32ToHexString(last_sender_id) + "\",";  // Sender NodeID as HEX string
  postData += "\"destination_nodeid\":\"" + uint32ToHexString(last_destination_id) + "\",";  // Destination NodeID as HEX string
  postData += "\"full_packet_len\":" + String(getLastFullPacketLen()) + ",";  // Full packet length including headers
  postData += "\"signal_level_dbm\":" + String(loraRssi);  // Signal strength in dBm
  postData += "}";
  String contentType = "application/json";
  ESP_LOGI(TAG, "Using Flask server with enhanced JSON POST data: sender_nodeid=%s, destination_nodeid=%s",
           uint32ToHexString(last_sender_id).c_str(), uint32ToHexString(last_destination_id).c_str());
#else
  // PHP server - form-encoded format
  String postData = "api_key=" + apiKey +
                    "&user_id=" + userId +
                    "&user_location=" + userLocation +
                    "&cold=" + String(cold_value) +
                    "&hot=" + String(hot_value) +
                    "&alarm_time=" + String(alarm_value);
  String contentType = "application/x-www-form-urlencoded";
  ESP_LOGI(TAG, "Using PHP server with form-encoded POST data");
#endif

  ESP_LOGD(TAG, "Preparing POST: cold=%ld, hot=%ld, path=%s, server=%s",
           cold_value, hot_value, path.c_str(), serverIP.c_str());
#if USE_HTTPS
  String nipIoUrl = getNipIoUrl(serverIP, port, serverPath);
  #if USE_INSECURE_HTTPS
  ESP_LOGI(TAG, "CURL example: curl -k -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d '%s' %s",
           postData.c_str(), nipIoUrl.c_str());
  #else
  ESP_LOGI(TAG, "CURL example: curl -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d '%s' %s",
           postData.c_str(), nipIoUrl.c_str());
  #endif
#else
  ESP_LOGI(TAG, "CURL example: curl -X POST -H 'Content-Type: application/x-www-form-urlencoded' -d '%s' %s://%s:%d/%s",
           postData.c_str(), serverProtocol.c_str(), serverIP.c_str(), port, serverPath.c_str());
#endif
  ESP_LOGI(TAG, "Full postData: %s", postData.c_str());

  lastHttpResult = "Sending POST with cold=" + String(cold_value) + ", hot=" + String(hot_value) + "...";

  if (WiFi.status() != WL_CONNECTED) {
    ESP_LOGE(TAG, "WiFi not connected, cannot send POST");
    lastHttpResult = "WiFi not connected";
    return;
  }

  ESP_LOGI(TAG, "Connecting to server %s on port %d", serverIP.c_str(), port);

  // Log the full HTTP request being sent
  String fullRequest = "POST " + path + " HTTP/1.1\r\n";
  fullRequest += "Host: " + serverIP + "\r\n";
  fullRequest += "User-Agent: curl/7.81.0\r\n";
  fullRequest += "Content-Type: " + contentType + "\r\n";
  fullRequest += "Content-Length: " + String(postData.length()) + "\r\n";
  fullRequest += "Connection: close\r\n";
  fullRequest += "\r\n";
  fullRequest += postData;
  ESP_LOGI(TAG, "Full HTTP request being sent:\n%s", fullRequest.c_str());

  if (client.connect(serverIP.c_str(), port)) {
    client.setTimeout(5000); // Set timeout 5 seconds
    ESP_LOGI(TAG, "Connected to server for POST");
    ESP_LOGI(TAG, "Sending POST headers");
    client.println("POST " + path + " HTTP/1.1");
    client.println("Host: " + serverIP);
    client.println("User-Agent: curl/7.81.0");
    client.println("Content-Type: " + contentType);
    client.println("Content-Length: " + String(postData.length()));
    client.println("Connection: close");
    client.println();
    ESP_LOGI(TAG, "Sending POST data");
    client.println(postData);
    ESP_LOGI(TAG, "POST data sent, client connected: %d", client.connected());

    // Wait for response
    ESP_LOGI(TAG, "Waiting for server response...");
    delay(1000);  // Wait 1 second for response to start
    String response = "";
    unsigned long start = millis();
    bool responseStarted = false;
    while (client.connected() && (millis() - start < 8000)) {  // Increased timeout to 8 seconds
      if (client.available()) {
        if (!responseStarted) {
          ESP_LOGI(TAG, "Server response started");
          responseStarted = true;
        }
        int len = client.available();
        for (int i = 0; i < len; i++) {
          char c = client.read();
          response += c;
          if (response.endsWith("\r\n\r\n")) {
            ESP_LOGI(TAG, "HTTP headers received, response length: %d", response.length());
            break; // End of HTTP headers
          }
        }
        start = millis(); // Reset timeout if data received
      }
      delay(10); // Yield
    }

    if (!responseStarted) {
      ESP_LOGW(TAG, "No response received from server within timeout");
    }

    ESP_LOGI(TAG, "Response received, total length: %d", response.length());

    if (response.length() == 0) {
      ESP_LOGE(TAG, "No response data, possible server timeout or rejection");
    }
    if (response.length() > 0) {
      ESP_LOGI(TAG, "Response content: %s", response.substring(0, 100).c_str());
    }
    client.stop();

    if (response.indexOf("200") >= 0) {
#if USE_FLASK_SERVER
      lastHttpResult = "Success: HTTP 200 (Flask JSON sent: sender_nodeid=" + uint32ToHexString(last_sender_id) +
                      ", destination_nodeid=" + uint32ToHexString(last_destination_id) +
                      ", full_packet_len=" + String(cold_value) +
                      ", signal_level_dbm=" + String(loraRssi) + ")";
      ESP_LOGI(TAG, "POST success, Flask JSON: sender_nodeid=%s, destination_nodeid=%s, full_packet_len=%ld, signal_level_dbm=%d",
               uint32ToHexString(last_sender_id).c_str(), uint32ToHexString(last_destination_id).c_str(), cold_value, loraRssi);
#else
      lastHttpResult = "Success: HTTP 200 (PHP form sent: cold=" + String(cold_value) + ", hot=" + String(hot_counter) + ")";
      ESP_LOGI(TAG, "POST success, PHP form: cold=%ld, hot=%lu", cold_value, hot_counter);
#endif
      hot_counter++;
    } else {
      lastHttpResult = "Failed: Server error";
      ESP_LOGE(TAG, "POST failed: response len=%d", response.length());
    }
  } else {
    lastHttpResult = "Failed: Cannot connect to server " + serverIP;
    ESP_LOGE(TAG, "Cannot connect to server %s", serverIP.c_str());
  }
}

void WiFiManager::pingTaskWrapper(void *param) {
  static_cast<WiFiManager*>(param)->pingTask();
}

void WiFiManager::startPingTask() {
  if (pingTaskHandle == nullptr) {
    xTaskCreate(pingTaskWrapper, "PingTask", 32768, this, 1, &pingTaskHandle);
  }
}

void WiFiManager::stopPingTask() {
  if (pingTaskHandle != nullptr) {
    vTaskDelete(pingTaskHandle);
    pingTaskHandle = nullptr;
  }
}

void WiFiManager::pingServer() {
#if USE_HTTPS
  WiFiClientSecure client;
  #if USE_INSECURE_HTTPS
  client.setInsecure(); // Skip certificate verification (equivalent to curl -k)
  #endif
#else
  WiFiClient client;
#endif
  int port = serverPort.toInt();
  ESP_LOGI(TAG, "Pinging server %s on port %d", serverIP.c_str(), port);
  if (client.connect(serverIP.c_str(), port)) {
    ESP_LOGI(TAG, "Ping success: connected to %s:%d", serverIP.c_str(), port);
    client.stop();
  } else {
    ESP_LOGE(TAG, "Ping failed: cannot connect to %s:%d", serverIP.c_str(), port);
  }
}

void WiFiManager::pingTask() {
  while (true) {
    if (enabled && isConnected()) {
      pingServer();
    } else {
      ESP_LOGD(TAG, "Ping: waiting... enabled=%d, connected=%d", enabled, isConnected());
    }
    vTaskDelay(pdMS_TO_TICKS(PING_INTERVAL_MS));
  }
}

void WiFiManager::httpPostTask() {
  while (true) {
    // First, process any queued POST requests
    processPostQueue();

    // Then handle periodic POSTs if enabled and not in LoRa trigger mode
    if (!post_on_lora_mm) {
      bool cond1 = enabled;
      bool cond2 = isConnected();
      bool cond3 = postEnabled;
      if (cond1 && cond2 && cond3) {
        ESP_LOGI(TAG, "POST Task: conditions met, sending periodic POST");
        doHttpPost();
      } else {
        ESP_LOGD(TAG, "POST Task: waiting... enabled=%d, connected=%d, postEnabled=%d, queue_size=%d",
                 cond1, cond2, cond3, postQueue.size());
      }
      vTaskDelay(pdMS_TO_TICKS(POST_INTERVAL_MS));
    } else {
      // In LoRa trigger mode, just process queue and wait
      ESP_LOGD(TAG, "POST Task: LoRa trigger mode, queue_size=%d", postQueue.size());
      vTaskDelay(pdMS_TO_TICKS(500));  // Check queue every 500ms
    }
  }
}

String WiFiManager::uint32ToHexString(uint32_t value) const {
  char hexStr[9];  // 8 chars + null terminator
  sprintf(hexStr, "%08X", value);
  return String(hexStr);
}

String WiFiManager::getLastSenderIdHex() const {
  return uint32ToHexString(last_sender_id);
}

String WiFiManager::getLastDestinationIdHex() const {
  return uint32ToHexString(last_destination_id);
}

// POST queue management
void WiFiManager::queuePostRequest(const String& postData) {
  if (postQueue.size() >= MAX_QUEUE_SIZE) {
    ESP_LOGW(TAG, "POST queue full (%d), dropping oldest request", MAX_QUEUE_SIZE);
    postQueue.erase(postQueue.begin());  // Remove oldest
  }
  postQueue.push_back(postData);
  ESP_LOGI(TAG, "Queued POST request, queue size: %d", postQueue.size());
}

void WiFiManager::processPostQueue() {
  if (postQueue.empty()) {
    return;
  }

  if (!isConnected()) {
    ESP_LOGD(TAG, "Not connected, cannot process queue");
    return;
  }

#if POST_BATCH_ENABLED
  // If batch mode enabled and we have enough items, send as batch
  if (postQueue.size() >= BATCH_SIZE) {
    sendBatchPost();
    return;
  }
#endif

  // Send individual requests (when batch disabled or queue is small)
  String postData = postQueue.front();
  postQueue.erase(postQueue.begin());

  ESP_LOGI(TAG, "Processing individual queued POST, %d remaining in queue", postQueue.size());

  // Send the queued request
  doHttpPostFromData(postData);
}

void WiFiManager::sendBatchPost() {
  if (postQueue.size() < BATCH_SIZE) {
    ESP_LOGW(TAG, "Not enough items for batch (%d < %d)", postQueue.size(), BATCH_SIZE);
    return;
  }

#if USE_FLASK_SERVER
  // Create JSON array with first BATCH_SIZE items
  String batchData = "[";

  for (size_t i = 0; i < BATCH_SIZE; i++) {
    if (i > 0) batchData += ",";
    batchData += postQueue[i];
  }
  batchData += "]";

  // Remove the batched items from queue
  postQueue.erase(postQueue.begin(), postQueue.begin() + BATCH_SIZE);

  ESP_LOGI(TAG, "Sending batch POST with %d items, %d remaining in queue", BATCH_SIZE, postQueue.size());

  // Send the batch
  doHttpPostFromData(batchData);
#else
  // For PHP server, fall back to individual requests
  ESP_LOGW(TAG, "Batch sending not supported for PHP server, sending individually");
  for (size_t i = 0; i < BATCH_SIZE && !postQueue.empty(); i++) {
    String postData = postQueue.front();
    postQueue.erase(postQueue.begin());
    doHttpPostFromData(postData);
  }
#endif
}

// Convert IP address to nip.io format for HTTPS public access
String WiFiManager::getNipIoUrl(String ip, int port, String path) {
  String nipIp = ip;
  nipIp.replace(".", "-");
  return "https://" + nipIp + ".nip.io:" + String(port) + path;
}

#if WIFI_DEBUG_FIXES
void WiFiManager::WiFiEvent(WiFiEvent_t event) {
  switch (event) {
    case SYSTEM_EVENT_STA_DISCONNECTED:
      ESP_LOGI(TAG, "WiFi lost connection. Reconnecting...");
      WiFi.reconnect();
      break;
    default:
      break;
  }
}
#endif
