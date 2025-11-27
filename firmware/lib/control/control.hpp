#pragma once

#include <Arduino.h>

#include <cstring>

#include "../lora_config.hpp"
#include "../wifi_manager/wifi_manager.hpp"
#include "LoRaCom.hpp"
#include "SerialCom.hpp"
#include "esp_log.h"

class Commander;  // Forward declaration
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "saveFlash.hpp"

#define c_cmp(a, b) (strcmp(a, b) == 0)

// this will be the data that is sent over LoRa in both directions on loop

// eg: "command update gain 22"
// eg: "status <deviceID> <RSSI> <batteryLevel> <mode> <status>"
// eg: "data <payload>"

// struct Data {
//   float force;
//   float timestamp;
// };

// struct Data {
//   String DataType;  // Type of data (e.g., "command", "data", "message")
//   String payload;   // The actual data being sent
//   int32_t RSSI;     // Received Signal Strength Indicator
//   String mode;      // mode of device (freq sweep receive, transceive,
//   transmit,
//                     // recieve, etc)
//   int8_t status;  // Status of the operation (e.g., all ok, error, warning,
//   low
//                   // battery, etc)
//   float batteryLevel;  // Battery level in volts
// };

class Control {
 public:
  Control();
  void setup();
  void begin();

  void setStatusEnabled(bool enabled) { statusEnabled = enabled; }

 private:
  SerialCom *m_serialCom;
  LoRaCom *m_LoRaCom;
  Commander *m_commander;
  SaveFlash *m_saveFlash;

  unsigned long serial_Interval = 100;
  unsigned long lora_Interval = 100;
  volatile bool statusEnabled = (bool)LORA_STATUS_ENABLED;  // Enable/disable periodic LoRa status sending

  static constexpr const char *TAG = "Control";

  TaskHandle_t SerialTaskHandle = nullptr;
  TaskHandle_t LoRaTaskHandle = nullptr;
  TaskHandle_t StatusTaskHandle = nullptr;

  WiFiManager *m_wifiManager;
  volatile bool post_on_lora = POST_EN_WHEN_LORA_RECEIVED;
  volatile bool wifi_enabled = WIFI_ENABLE;

  void serialDataTask();
  void loRaDataTask();
  void statusTask();

  void interpretMessage(const char *buffer, bool relayMsgLoRa = true);
  void processData(const char *buffer);

  String deviceID = "transceiver";  // Unique identifier for the device

  // Mode of operation (transmit, receive, transceive, etc.)
  String m_mode = "transceive";
  String m_status = "ok";  // Status of the device (e.g., "ok", "error", etc.)
  float m_batteryLevel = 100.0;  // Battery level as a percentage (0-100)

  // Data payload;
};
