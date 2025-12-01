#include <Arduino.h>
#include "../wifi_test_config.hpp"
#include "wifi_manager_simple.hpp"

WiFiManagerSimple* wifiManager = nullptr;

void setup() {
  Serial.begin(115200);
  delay(1000);

  ESP_LOGI("WiFiTest", "=== WiFi Power Consumption Test Started ===");
  ESP_LOGI("WiFiTest", "WiFi SSID: %s", DEFAULT_WIFI_SSID);
  ESP_LOGI("WiFiTest", "POST_HOT_VALUE=%d, POST_COLD_VALUE=%d", POST_HOT_VALUE, POST_COLD_VALUE);
  ESP_LOGI("WiFiTest", "POST_INTERVAL_MS=%d", POST_INTERVAL_MS);

  // Initialize WiFi manager
  wifiManager = new WiFiManagerSimple();

  // Try to connect to WiFi
  if (wifiManager->connect()) {
    ESP_LOGI("WiFiTest", "WiFi connected successfully!");
#if defined(STATUS_LED_PIN)
    pinMode(STATUS_LED_PIN, OUTPUT);
    digitalWrite(STATUS_LED_PIN, HIGH); // Turn on LED when connected
#endif
  } else {
    ESP_LOGE("WiFiTest", "WiFi connection failed! Check your settings.");
    ESP_LOGI("WiFiTest", "Make sure to update network_definitions.h with your WiFi credentials");
  }
}

void loop() {
  static unsigned long lastPostTime = 0;
  unsigned long currentTime = millis();

  // Check WiFi connection status
  bool isConnected = wifiManager->isConnected();

#if defined(STATUS_LED_PIN)
  digitalWrite(STATUS_LED_PIN, isConnected ? HIGH : LOW);
#endif

  // Send POST every POST_INTERVAL_MS
  if (isConnected && (currentTime - lastPostTime >= POST_INTERVAL_MS)) {
    ESP_LOGI("WiFiTest", "Time to send POST (interval: %d ms)", POST_INTERVAL_MS);
    wifiManager->doHttpPost();
    lastPostTime = currentTime;
  } else if (!isConnected) {
    ESP_LOGW("WiFiTest", "WiFi not connected, attempting reconnection...");
    if (wifiManager->connect()) {
      ESP_LOGI("WiFiTest", "Reconnected successfully!");
    }
    delay(5000); // Wait 5 seconds before retry
  }

  // Short delay to prevent busy loop
  delay(1000);

  // Log statistics every 60 seconds
  static unsigned long lastStatsTime = 0;
  if (currentTime - lastStatsTime >= 60000) {
    ESP_LOGI("WiFiTest", "=== STATUS CHECK ===");
    ESP_LOGI("WiFiTest", "Connected: %s", isConnected ? "YES" : "NO");
    if (isConnected) {
      ESP_LOGI("WiFiTest", "IP: %s", WiFi.localIP().toString().c_str());
      ESP_LOGI("WiFiTest", "RSSI: %d dBm", WiFi.RSSI());
    }
    ESP_LOGI("WiFiTest", "Current hot/cold values: %d/%d",
             wifiManager->getHotValue(), wifiManager->getColdValue());
    lastStatsTime = currentTime;
  }
}
