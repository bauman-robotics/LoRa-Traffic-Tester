#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <vector>
#include "../lora_config.hpp"

class WiFiManager {
 public:
  static constexpr const char *TAG = "WiFiManager";

  WiFiManager();
  void loadSettings();
  bool connect();
  void disconnect();
  bool isConnected();
  void enable(bool state);
  bool isEnabled() { return enabled; }
  void setLastLoRaPacketLen(int len);
  int getLastLoRaPacketLen();
  void enablePost(bool state);
  bool isPostEnabled() { return postEnabled; }
  void setSendPostOnLoRa(bool value) { sendPostOnLoRa = value; }
  void setPostOnLora(bool value);
  void setLoRaRssi(int32_t rssi) { loraRssi = rssi; }
  void setLastSenderId(int id) { last_sender_id = id; }
  void setLastDestinationId(int id) { last_destination_id = id; }
  void setLastFullPacketLen(int len) { lastFullPacketLen = len; }
  int32_t getLastFullPacketLen() { return lastFullPacketLen; }
  String getSSID() const { return ssid; }
  String getPassword() const { return password; }
  String getAPIKey() const { return apiKey; }
  String getServerURL() const { return serverProtocol + "://" + serverIP + "/" + serverPath; }
  String getLastHttpResult() const { return lastHttpResult; }
  String getUserId() const { return userId; }
  String getUserLocation() const { return userLocation; }
  int32_t getLastSenderId() const { return last_sender_id; }
  String getLastSenderIdHex() const;
  int32_t getLastDestinationId() const { return last_destination_id; }
  String getLastDestinationIdHex() const;
  int32_t getLastRssi() const { return loraRssi; }
  void sendSinglePost();

  // Statistics getters and incrementers
  unsigned long getLoraPacketsReceived() const { return loraPacketsReceived; }
  void incrementLoraPacketsReceived() { loraPacketsReceived++; }
  unsigned long getPostRequestsSent() const { return postRequestsSent; }
  void incrementPostRequestsSent() { postRequestsSent++; }

  // POST queue management
  void queuePostRequest(const String& postData);
  void processPostQueue();
  void sendBatchPost();
  size_t getQueueSize() const { return postQueue.size(); }

  // WiFi credentials persistence
  void saveWiFiCredentials();
  void loadWiFiCredentials();
  void setWiFiCredentials(const String& new_ssid, const String& new_password);

 private:
  void httpPostTask();
  static void httpPostTaskWrapper(void *param);
  void startPOSTTask();
  void stopPOSTTask();
  void doHttpPost();
  void doHttpPostFromData(const String& postData);

  void pingTask();
  static void pingTaskWrapper(void *param);
  void startPingTask();
  void stopPingTask();
  void pingServer();

  String uint32ToHexString(uint32_t value) const;  // Convert uint32_t to 8-character hex string
  String getNipIoUrl(String ip, int port, String path);  // Convert IP to nip.io format

  String ssid, password;
  String apiKey, userId, userLocation;
  String serverProtocol, serverIP, serverPort, serverPath;
  bool enabled = false;
  bool postEnabled = POST_INTERVAL_EN;
  volatile bool sendPostOnLoRa = false;
  volatile bool post_on_lora_mm = POST_EN_WHEN_LORA_RECEIVED;
  int32_t loraRssi = 0;
  int32_t last_sender_id = ALARM_TIME;
  int32_t last_destination_id = 0xFFFFFFFF;  // Destination NodeID for Flask server (defaults to broadcast)
  int32_t lastFullPacketLen = 0;  // Full packet length including headers for Flask server
  TaskHandle_t httpTaskHandle = nullptr;
  TaskHandle_t pingTaskHandle = nullptr;
  String lastHttpResult = "No posts yet";

  // POST request queue
  std::vector<String> postQueue;
  static const size_t MAX_QUEUE_SIZE = 10;  // Maximum queued requests
  static const size_t BATCH_SIZE = 5;       // Send batched requests when queue reaches this size

  // Statistics counters
  volatile unsigned long loraPacketsReceived = 0;  // Total LoRa packets received
  volatile unsigned long postRequestsSent = 0;     // Total POST requests successfully sent

#if WIFI_DEBUG_FIXES
  static void WiFiEvent(WiFiEvent_t event);
#endif
};
