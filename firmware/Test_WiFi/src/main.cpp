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
    delay(10000); // Wait 5 seconds before retry
  }

  // Short delay to prevent busy loop
  delay(1000);

}
