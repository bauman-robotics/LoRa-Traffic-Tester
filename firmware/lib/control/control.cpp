#include "control.hpp"
#include "commander.hpp"

void* wifi_manager_global = nullptr;
volatile bool force_lora_trigger = false;

// Initialize status interval from macro
unsigned long status_Interval = LORA_STATUS_INTERVAL_SEC * 1000;  // in ms

// Initialize cold counter
unsigned long cold_counter = COLD_INITIAL;

// Initialize hot counter (successful POSTs)
unsigned long hot_counter = 0;

// Remove postMode for stability

Control::Control() {
  m_serialCom = new SerialCom();  // Initialize SerialCom instance
  m_LoRaCom = new LoRaCom();      // Initialize LoRaCom instance
  m_commander =
      new Commander(m_serialCom, m_LoRaCom, this);  // Initialize Commander instance

  //m_saveFlash = new SaveFlash(m_serialCom);  // Initialize SaveFlash instance
  m_wifiManager = new WiFiManager();         // Initialize WiFiManager instance
  wifi_manager_global = m_wifiManager;       // Set global pointer
}

void Control::setup() {
  // Disable WiFi auto reconnect to manual control
  WiFi.setAutoReconnect(false);
  ESP_LOGI(TAG, "WiFi auto reconnect disabled");

  // Set MAC address for WiFi STA
#if WIFI_USE_FIXED_MAC
  uint8_t fixed_mac[6] = WIFI_FIXED_MAC_ADDRESS;
  esp_base_mac_addr_set(fixed_mac);
  ESP_LOGI(TAG, "Fixed MAC set: %02X:%02X:%02X:%02X:%02X:%02X", fixed_mac[0], fixed_mac[1], fixed_mac[2], fixed_mac[3], fixed_mac[4], fixed_mac[5]);
#elif WIFI_USE_CUSTOM_MAC
  uint8_t base_mac[6];
  esp_read_mac(base_mac, ESP_MAC_WIFI_STA);
  uint8_t custom_mac[6];
  for (int i = 0; i < 6; i++) {
    custom_mac[i] = (uint8_t)random(0, 256);
  }
  esp_base_mac_addr_set(custom_mac);
  ESP_LOGI(TAG, "Custom random MAC set: %02X:%02X:%02X:%02X:%02X:%02X", custom_mac[0], custom_mac[1], custom_mac[2], custom_mac[3], custom_mac[4], custom_mac[5]);
#endif

  m_serialCom->init(115200);  // Initialize serial communication

  bool loraSuccess = true;
  if (!FAKE_LORA) {
    loraSuccess =
        m_LoRaCom->begin<SX1262>(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS,
                                 LORA_DIO1, LORA_RESET, LORA_FREQUENCY, LORA_POWER, LORA_BUSY);
    if (!loraSuccess) {
      ESP_LOGW(TAG, "LoRa initialization FAILED! Check your hardware connections.");
      ESP_LOGW(TAG, "***FALLBACK TO FAKE LoRa MODE - HARDWARE NOT DETECTED***");
      loraSuccess = m_LoRaCom->beginFake(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS,
                                          LORA_DIO1, LORA_RESET, LORA_FREQUENCY, LORA_POWER, LORA_BUSY);
    }
  } else {
    ESP_LOGI(TAG, "FAKE_LORA=1: Skipping hardware LoRa initialization, using fake mode");
    loraSuccess = m_LoRaCom->beginFake(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS,
                                        LORA_DIO1, LORA_RESET, LORA_FREQUENCY, LORA_POWER, LORA_BUSY);
  }

  if (!loraSuccess) {
    ESP_LOGE(TAG,
             "LoRa initialization FAILED! Check your hardware connections.");
    ESP_LOGE(TAG,
             "Pin assignments: CLK=%d, MISO=%d, MOSI=%d, CS=%d, INT=%d, "
             "RST=%d, BUSY=%d",
             LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS, LORA_DIO1, LORA_RESET,
             LORA_BUSY);
  } else {
    ESP_LOGI(TAG, "LoRa initialized successfully!");

    // Set MESH parameters if enabled
    if (MESH_COMPATIBLE && !FAKE_LORA) {
      m_LoRaCom->setSpreadingFactor(MESH_SPREADING_FACTOR);
      m_LoRaCom->setBandwidth(MESH_BANDWIDTH);
      m_LoRaCom->setCodingRate(MESH_CODING_RATE);
      m_LoRaCom->setSyncWord(MESH_SYNC_WORD);
      ESP_LOGI(TAG, "MESH mode enabled, set SF=%d, BW=%d, CR=%d, SW=0x%02X", MESH_SPREADING_FACTOR, MESH_BANDWIDTH, MESH_CODING_RATE, MESH_SYNC_WORD);
    }

    // Log current settings
    if (!FAKE_LORA) {
      int currentSF = m_LoRaCom->getCurrentSF();
      int currentBW = m_LoRaCom->getCurrentBW();
      int currentCR = m_LoRaCom->getCurrentCR();
      ESP_LOGI(TAG, "LoRa settings: frequency=%0.3f MHz, power=%d dBm, SF=%d, BW=%d kHz, CR=%d", LORA_FREQUENCY, LORA_POWER, currentSF, currentBW, currentCR);
    } else {
      ESP_LOGI(TAG, "LoRa FAKE mode: frequency=%0.3f MHz, power=%d dBm", LORA_FREQUENCY, LORA_POWER);
    }

    ESP_LOGI(TAG, "POST settings: POST_EN_WHEN_LORA_RECEIVED=%d, POST_HOT_AS_RSSI=%d", POST_EN_WHEN_LORA_RECEIVED, POST_HOT_AS_RSSI);
    ESP_LOGI(TAG, "MESH settings: MESH_COMPATIBLE=%d", MESH_COMPATIBLE);
    if (MESH_COMPATIBLE) {
      ESP_LOGI(TAG, "  MESH_SYNC_WORD=0x%02X", MESH_SYNC_WORD);
      ESP_LOGI(TAG, "  MESH_FREQUENCY=%.3f MHz", MESH_FREQUENCY);
      ESP_LOGI(TAG, "  MESH_BANDWIDTH=%d kHz", MESH_BANDWIDTH);
      ESP_LOGI(TAG, "  MESH_SPREADING_FACTOR=%d", MESH_SPREADING_FACTOR);
      ESP_LOGI(TAG, "  MESH_CODING_RATE=%d", MESH_CODING_RATE);
    }
  }

  //m_saveFlash->begin();  // Initialize flash storage

  m_wifiManager->loadSettings();

  // Auto enable WiFi if configured
  if (WIFI_ENABLE) {
    m_wifiManager->enable(true);
  } else {
    wifi_enabled = false;
  }

  // DIO1 setup not needed without light sleep
}

void Control::begin() {
  // Begin method implementation
  ESP_LOGI(TAG, "Control beginning...");

  // Delete the previous tasks if they exist
  if (SerialTaskHandle != nullptr) {
    vTaskDelete(SerialTaskHandle);
  }

  if (LoRaTaskHandle != nullptr) {
    vTaskDelete(LoRaTaskHandle);
  }

  if (StatusTaskHandle != nullptr) {
    vTaskDelete(StatusTaskHandle);
  }

  // Create new tasks for serial data handling, LoRa data handling, and status
  // Higher priority = higher number, priorities should be 1-3 for user tasks
  xTaskCreate(
      [](void *param) { static_cast<Control *>(param)->serialDataTask(); },
      "SerialDataTask", 8192, this, 2, &SerialTaskHandle);

  xTaskCreate(
      [](void *param) { static_cast<Control *>(param)->loRaDataTask(); },
      "LoRaDataTask", 8192, this, 2, &LoRaTaskHandle);

  xTaskCreate([](void *param) { static_cast<Control *>(param)->statusTask(); },
              "StatusTask", 8192, this, 1, &StatusTaskHandle);

  ESP_LOGI(TAG, "Control begun!\n");

  ESP_LOGI(TAG, "Type <help> for a list of commands");
}

void Control::serialDataTask() {
  char buffer[128];  // Buffer to store incoming data
  int rxIndex = 0;   // Index to track the length of the received message

  while (true) {
    // Check for incoming data from the serial interface
    if (m_serialCom->getData(buffer, sizeof(buffer), &rxIndex)) {
      ESP_LOGI(TAG, "Serial received: %s", buffer);  // Log the received data
      interpretMessage(buffer, true);         // Process the message
      // clear the buffer for the next message
      memset(buffer, 0, sizeof(buffer));
      rxIndex = 0;  // Reset the index
    }

    // Use a small delay instead of yield() to be more cooperative
    vTaskDelay(pdMS_TO_TICKS(10));  // 10ms delay allows other tasks to run
  }
}

void Control::loRaDataTask() {
  char buffer[256];  // Buffer to store incoming data, increased to handle large Meshtastic packets

  while (true) {
    // Always call getMessage to process any pending transmissions/receptions
    int receivedLen = 0;  // Initialize to track actual received length
    if (m_LoRaCom->getMessage(buffer, sizeof(buffer), &receivedLen)) {
      ESP_LOGI(TAG, "LoRa packet received, length: %d bytes", receivedLen);
      // Log first few bytes in hex for debugging
      if (receivedLen > 0) {
        char hexBuf[64];
        int hexLen = min(16, receivedLen);  // Show first 16 bytes
        sprintf(hexBuf, "First %d bytes (hex): ", hexLen);
        for (int i = 0; i < hexLen; i++) {
          sprintf(hexBuf + strlen(hexBuf), "%02X ", (uint8_t)buffer[i]);
        }
        ESP_LOGD(TAG, "%s", hexBuf);
      }

      if (OLD_LORA_PARS) {
        // Old parsing method (simplified)
        ESP_LOGD(TAG, "Received: %s", buffer);  // Log the received data
        // Set packet length for POST
        int oldLength = strlen(buffer);
        ESP_LOGI(TAG, "LoRa packet payload length: %d", oldLength);
        m_wifiManager->setLastLoRaPacketLen(oldLength);
        //m_wifiManager->setLastSenderId(1);

        interpretMessage(buffer, false);        // Process the message
        // Send the received data over serial
        m_serialCom->sendData("LoRa Received: <");
        m_serialCom->sendData(buffer);
        m_serialCom->sendData(">\n");

        // Save to flash for logging all received LoRa messages
        //m_saveFlash->writeData((String("RX: ") + buffer + "\n").c_str());

        // POST on LoRa receive disabled for stability
        ESP_LOGI(TAG, "OLD_LORA_PARS=1: Simple processing completed, alarm_time=01");
      } else {
        // Extract sender ID from packet header - new logic
        if (receivedLen >= 8) {
          // const uint8_t* packetData = reinterpret_cast<const uint8_t*>(buffer);
          // // Sender ID at offset 0x04 (4 bytes, little-endian 32-bit integer)
          // //uint32_t senderId = packetData[4] | (packetData[5] << 8) | (packetData[6] << 16) | (packetData[7] << 24);
          // uint32_t senderId = packetData[0] | (packetData[1] << 8) | (packetData[2] << 16) | (packetData[3] << 24);
          // // Take last 2 bytes (mладшие 16 bits) of sender ID
          // uint16_t alarmValue = senderId & 0xFFFF;
          // m_wifiManager->setLastSenderId(senderId);
          // ESP_LOGI(TAG, "Extracted sender NodeID: %lu (alarm_time=%04X)", (unsigned long)senderId, alarmValue);
        } else {
          // Packet too short (< 8 bytes), can't extract sender ID
          //m_wifiManager->setLastSenderId(0);
          //ESP_LOGI(TAG, "Packet too short (%d bytes < 8), can't extract sender ID, alarm_time=00", receivedLen);
        }

        // Set packet length for POST - use actual received length
        m_wifiManager->setLastLoRaPacketLen(receivedLen);

        // Null-terminate for string operations if needed
        if (receivedLen < sizeof(buffer)) {
          buffer[receivedLen] = '\0';
        }

        // Log first few bytes in hex for debugging
        if (receivedLen > 0) {
          char hexBuf[64];
          int hexLen = min(16, receivedLen);  // Show first 16 bytes
          sprintf(hexBuf, "First %d bytes (hex): ", hexLen);
          for (int i = 0; i < hexLen; i++) {
            sprintf(hexBuf + strlen(hexBuf), "%02X ", (uint8_t)buffer[i]);
          }
          ESP_LOGD(TAG, "%s", hexBuf);
        }

        // Only try to interpret as text if it's mostly printable ASCII
        bool isTextMessage = true;
        // for (int i = 0; i < receivedLen && i < 50; i++) {
        //   if (buffer[i] < 32 && buffer[i] != '\n' && buffer[i] != '\r' && buffer[i] != '\t') {
        //     isTextMessage = false;
        //     break;
        //   }
        // }

        if (isTextMessage) {
          ESP_LOGD(TAG, "Received (text): %s", buffer);
          interpretMessage(buffer, false);        // Process the message
          // Send the received data over serial
          m_serialCom->sendData("LoRa Received: <");
          m_serialCom->sendData(buffer);
          m_serialCom->sendData(">\n");

          // Save to flash for logging all received LoRa messages
          //m_saveFlash->writeData((String("RX: ") + buffer + "\n").c_str());
        } else {
          ESP_LOGI(TAG, "Received binary packet (%d bytes), skipping text interpretation", receivedLen);
          // Save to flash as hex dump for binary packets
          char hexDump[512];
          sprintf(hexDump, "RX_BIN: %d bytes - ", receivedLen);
          int hexLen = min(32, receivedLen);  // Show first 32 bytes
          for (int i = 0; i < hexLen; i++) {
            sprintf(hexDump + strlen(hexDump), "%02X ", (uint8_t)buffer[i]);
          }
          ///m_saveFlash->writeData((String(hexDump) + "\n").c_str());
        }
      }

      memset(buffer, 0, sizeof(buffer));
    }

    // Reduce CPU wakeups when WiFi off: longer delay
    int delay_ms = wifi_enabled ? 10 : 100;
    vTaskDelay(pdMS_TO_TICKS(delay_ms));
  }
}

void Control::statusTask() {
  static unsigned long lastStatusTime = 0;
  static uint16_t packetCounter = 0;  // Counter for short packets

  while (true) {
    if (statusEnabled) {
      // Process any pending LoRa operations first
      // m_LoRaCom->processOperations();

      String msg;
      if (!LORA_STATUS_SHORT_PACKETS) {
        // Full packet: st ID:transceiver R:-63 B:100.00 M:transceive S:ok
        int32_t rssi = m_LoRaCom->getRssi();
        msg = String("st ") + "ID:" + deviceID +
              " R:" + String(rssi) +
              " B:" + String(m_batteryLevel) + " M:" + m_mode +
              " S:" + m_status;
      } else {
        // Short packet: 2-byte hex counter, e.g. "00", "01"...
        char buf[3];
        sprintf(buf, "%02X", packetCounter++ % 256);  // Wrap at 255
        msg = String(buf);
      }

      // Send over serial first (this should be fast)
      m_serialCom->sendData(((msg + "\n").c_str()));

      // Send over LoRa
      ESP_LOGD(TAG, "Sending status over LoRa: %s", msg.c_str());
      m_LoRaCom->sendMessage(msg.c_str());

      // Wait for LoRa transmission to complete
      while (m_LoRaCom->checkTxMode()) {
        vTaskDelay(pdMS_TO_TICKS(10));  // Wait for LoRa transmission
      }
    }
    vTaskDelay(pdMS_TO_TICKS(status_Interval));
  }
}

void Control::interpretMessage(const char *buffer, bool relayMsgLoRa) {
  m_commander->setCommand(buffer);  // Set the command in the commander
  char *token = m_commander->readAndRemove();

  // eg: "command update gain 22"
  // eg: "status <deviceID> <RSSI> <batteryLevel> <mode> <status>"
  // eg: "data <payload>"

  if (token != nullptr && c_cmp(token, "command")) {
    // Check for special commands like set status
    m_commander->setCommand(buffer);
    char *cmd_token = m_commander->readAndRemove();  // "command"
    cmd_token = m_commander->readAndRemove();  // subcommand
    if (cmd_token != nullptr && c_cmp(cmd_token, "set")) {
      cmd_token = m_commander->readAndRemove();  // parameter
      if (cmd_token != nullptr && c_cmp(cmd_token, "status")) {
        cmd_token = m_commander->readAndRemove();  // value
        if (cmd_token) {
          bool state = (atoi(cmd_token) == 1);
          setStatusEnabled(state);
          ESP_LOGI(TAG, "Status sending set to %s", state ? "ON" : "OFF");

          // Send first status packet immediately when enabled
          if (state) {
            String msg;
            if (!LORA_STATUS_SHORT_PACKETS) {
              int32_t rssi = m_LoRaCom->getRssi();
              msg = String("st ") + "ID:" + deviceID +
                    " R:" + String(rssi) +
                    " B:" + String(m_batteryLevel) + " M:" + m_mode +
                    " S:" + m_status;
            } else {
              static uint16_t initCounter = 0;  // For initial packet
              char buf[3];
              sprintf(buf, "%02X", initCounter++);
              msg = String(buf);
            }

            // Send over serial first
            m_serialCom->sendData(((msg + "\n").c_str()));

            // Send over LoRa
            ESP_LOGD(TAG, "Sending initial status over LoRa: %s", msg.c_str());
            m_LoRaCom->sendMessage(msg.c_str());

            // Wait for LoRa transmission to complete
            while (m_LoRaCom->checkTxMode()) {
              vTaskDelay(pdMS_TO_TICKS(10));
            }
          }

          return;  // Handled
        }
      } else if (c_cmp(cmd_token, "interval")) {
        cmd_token = m_commander->readAndRemove();  // value
        if (cmd_token) {
          int interval_sec = atoi(cmd_token);
          status_Interval = interval_sec * 1000;  // Convert to milliseconds
          ESP_LOGI(TAG, "Status interval set to %d seconds", interval_sec);
          return;  // Handled
        }
      } else if (c_cmp(cmd_token, "wifi_en")) {
        cmd_token = m_commander->readAndRemove();  // value
        if (cmd_token) {
          bool state = (atoi(cmd_token) == 1);
          m_wifiManager->enable(state);
          wifi_enabled = state;
          return;  // Handled
        }
      } else if (c_cmp(cmd_token, "post_en")) {
        cmd_token = m_commander->readAndRemove();  // value
        if (cmd_token) {
          bool state = (atoi(cmd_token) == 1);
          m_wifiManager->enablePost(state);
          return;  // Handled
        }
      } else if (c_cmp(cmd_token, "post_mode")) {
        cmd_token = m_commander->readAndRemove();  // "lora" or "time"
        if (cmd_token) {
          if (c_cmp(cmd_token, "lora")) {
            post_on_lora = true;
            force_lora_trigger = true;
          } else {
            post_on_lora = false;
            force_lora_trigger = false;
          }
          cold_counter = COLD_INITIAL;
          hot_counter = 0;
          m_wifiManager->setPostOnLora(post_on_lora);
          ESP_LOGI(TAG, "Counters reset: cold=%lu, hot=%lu", cold_counter, hot_counter);
          ESP_LOGI(TAG, "POST mode set to %s", post_on_lora ? "LoRa receive trigger" : "Periodic");
          return;  // Handled
        }
      }
    }
    // Reset command for normal processing (skip "command " prefix)
    const char* cmd_start = buffer;
    if (strncmp(cmd_start, "command ", 8) == 0) {
      cmd_start += 8;  // Skip "command " prefix
    }
    m_commander->setCommand(cmd_start);
    if (relayMsgLoRa) {
      // send to other devices to sync parameters
      m_LoRaCom->sendMessage(buffer);
      while (m_LoRaCom->checkTxMode()) {
        vTaskDelay(pdMS_TO_TICKS(10));  // Wait for LoRa transmission
      }
    }
    // should probably wait for a success reply before changing THIS device
    ESP_LOGD(TAG, "Processing command: %s", cmd_start);
    m_commander->checkCommand();
  } else if (token != nullptr && c_cmp(token, "data")) {
    processData(buffer);
  } else if (token != nullptr && c_cmp(token, "get")) {
    m_commander->setCommand(buffer);
    char *get_token = m_commander->readAndRemove(); // "get"
    get_token = m_commander->readAndRemove(); // "wifi_status" or "http_status"
    if (get_token != nullptr && c_cmp(get_token, "wifi_status")) {
      int status = m_wifiManager->isConnected() ? 1 : 0;
      String msg = "wifi_status " + String(status) + "\n";
      m_serialCom->sendData(msg.c_str());
      return;
    } else if (get_token != nullptr && c_cmp(get_token, "http_status")) {
      String result = m_wifiManager->getLastHttpResult();
      m_serialCom->sendData(("http_status " + result + "\n").c_str());
      return;
      } else if (get_token != nullptr && c_cmp(get_token, "debug_info")) {
        // Send debug info as separate serial messages
        char buf[128];
        sprintf(buf, "wifi_ssid: %s\n", m_wifiManager->getSSID().c_str());
        m_serialCom->sendData(buf);
        sprintf(buf, "api_key: %s\n", m_wifiManager->getAPIKey().c_str());
        m_serialCom->sendData(buf);
        sprintf(buf, "server_url: %s\n", m_wifiManager->getServerURL().c_str());
        m_serialCom->sendData(buf);
        sprintf(buf, "hot_water: %lu\n", hot_counter);
        m_serialCom->sendData(buf);
        sprintf(buf, "alarm_time: %d\n", ALARM_TIME);
        m_serialCom->sendData(buf);
        sprintf(buf, "post_interval_ms: %d\n", POST_INTERVAL_MS);
        m_serialCom->sendData(buf);
        sprintf(buf, "cold_initial: %d\n", COLD_INITIAL);
        m_serialCom->sendData(buf);
        // Removed postMode
        return;
      } else if (get_token != nullptr && c_cmp(get_token, "gain")) {
        m_serialCom->sendData(("gain " + String(LORA_POWER) + "\n").c_str());
        return;
      } else if (c_cmp(get_token, "freq")) {
        m_serialCom->sendData(("freq " + String(LORA_FREQUENCY, 3) + "\n").c_str());
        return;
      } else if (c_cmp(get_token, "sf")) {
        m_serialCom->sendData(("sf " + String(m_LoRaCom->getCurrentSF()) + "\n").c_str());
        return;
      } else if (c_cmp(get_token, "bw")) {
        m_serialCom->sendData(("bw " + String(m_LoRaCom->getCurrentBW()) + "\n").c_str());
        return;
      } else if (c_cmp(get_token, "status")) {
        m_serialCom->sendData(("status " + String(statusEnabled ? 1 : 0) + "\n").c_str());
        return;
      } else if (c_cmp(get_token, "interval")) {
        m_serialCom->sendData(("interval " + String(status_Interval / 1000) + "\n").c_str());
        return;
      } else if (c_cmp(get_token, "wifi_en")) {
        m_serialCom->sendData(("wifi_en " + String(wifi_enabled ? 1 : 0) + "\n").c_str());
        return;
      } else if (c_cmp(get_token, "post_mode")) {
        m_serialCom->sendData(("post_mode " + String(post_on_lora ? "lora" : "time") + "\n").c_str());
        return;
      } else if (c_cmp(get_token, "post_en")) {
        m_serialCom->sendData(("post_en " + String(POST_EN_WHEN_LORA_RECEIVED) + "\n").c_str());
        return;
    }
  } else if (token != nullptr && c_cmp(token, "status")) {
    processData(buffer);
  } else if (token != nullptr && c_cmp(token, "help")) {
    ESP_LOGI(TAG,
             "Message format: <type> <data1> <data2> ...\n"
             "Valid types:\n"
             "  - flash: for flash read and save\n"
             "  - command: for device control\n"
             "  - data: for data transmission\n"
             "  - message: for standard messages\n"
             "  - flash: to print and auto erase logs\n"
             "  - status: for device status\n"
             "  - help: for displaying help information");
  } else if (token != nullptr && c_cmp(token, "flash")) {
    //m_saveFlash->readFile();
    //m_saveFlash->removeFile();  // Update the flash storage
    //m_saveFlash->begin();       // Reinitialize the flash storage
  }
}

void Control::processData(const char *buffer) {
  // Process the data message - send to LoRa
  ESP_LOGD(TAG, "Processing data for LoRa transmission");

  // remove the "data" prefix
  const char *dataStart = strchr(buffer, ' ') + 1;  // Find the first space
  if (dataStart == nullptr) {
    ESP_LOGE(TAG, "Invalid data format: %s", buffer);
    return;  // Invalid format, return early
  }

  // Send to LoRa
  m_LoRaCom->sendMessage(dataStart);

  // Send confirmation over serial
  m_serialCom->sendData("Data sent to LoRa: ");
  m_serialCom->sendData(dataStart);
  m_serialCom->sendData("Data sent to LoRa: ");

  // Also save to flash for logging
  //m_saveFlash->writeData((String("TX: ") + dataStart + "\n").c_str());

  ESP_LOGI(TAG, "Data sent to LoRa: %s", dataStart);
}
