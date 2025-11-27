#include "commander.hpp"
#include "control.hpp"

Commander::Commander(SerialCom* serialCom, LoRaCom* loraCom, Control* control) {
  memset(m_command, 0, sizeof(m_command));  // Initialize command buffer
  m_serialCom = serialCom;                  // Initialize the SerialCom instance
  m_loraCom = loraCom;                      // Initialize the LoRaCom instance
  m_control = control;                       // Initialize the Control instance
  ESP_LOGD(TAG, "Commander initialised");
}

void Commander::handle_command_help() {
  handle_help(command_handler);  // Call the generic help handler
}

void Commander::handle_update_help() {
  handle_help(update_handler);  // Call the generic help handler
}

void Commander::handle_set_help() {
  handle_help(set_handler);  // Call the generic help handler
}

void Commander::handle_help(const HandlerMap* handler) {
  String helpText = "\nAvailable commands:\n";
  for (const HandlerMap* cmd = handler; cmd->name != nullptr; ++cmd) {
    helpText +=
        "- <" + String(cmd->name) + ">\n";  // Append command names to help text
  }
  ESP_LOGI(TAG, "%s", helpText.c_str());
}

void Commander::handle_update() {
  ESP_LOGI(TAG, "Update command executed");
  checkCommand(update_handler);
}

void Commander::handle_set() {
  ESP_LOGD(TAG, "Set command executed");
  checkCommand(set_handler);  // Check and run the set command
}

void Commander::handle_get() {
  ESP_LOGD(TAG, "Get command executed");
  checkCommand(get_handler);  // Check and run the get command
}

void Commander::handle_update_gain() {
  ESP_LOGD(TAG, "Update gain command executing");

  char* data = readAndRemove();  // Read and remove the command token

  // convert to integer
  if (data == nullptr) {
    ESP_LOGW(TAG, "Empty data received for gain update, expecting <int8_t>");
    return;
  }

  int8_t gain = static_cast<int8_t>(atoi(data));  // Convert to int8_t
  m_loraCom->setOutGain(gain);                    // Set the gain in LoRaCom
}

void Commander::handle_update_freqMhz() {
  ESP_LOGD(TAG, "Update freqMhz command executing");

  char* data = readAndRemove();  // Read and remove the command token

  // convert to integer
  if (data == nullptr) {
    ESP_LOGW(TAG, "Empty data received for gain update, expecting <float>");
    return;
  }

  float freqMhz = static_cast<float>(atof(data));  // Cast to float
  m_loraCom->setFrequency(freqMhz);                // Set the gain in LoRaCom
}

void Commander::handle_update_spreadingFactor() {
  // Implementation for updating spreading factor
  ESP_LOGD(TAG, "Update spreading factor command executing");

  char* data = readAndRemove();  // Read and remove the command token

  // convert to integer
  if (data == nullptr) {
    ESP_LOGW(TAG, "Empty data received for spreading factor update, expecting <uint8_t>");
    return;
  }

  uint8_t spreadingFactor = static_cast<uint8_t>(atoi(data));
  ESP_LOGI(TAG, "Setting spreading factor to %d", spreadingFactor);
  if (m_loraCom->setSpreadingFactor(spreadingFactor)) {
    ESP_LOGI(TAG, "Spreading factor updated successfully to %d", spreadingFactor);
  } else {
    ESP_LOGE(TAG, "Failed to update spreading factor to %d", spreadingFactor);
  }
}

void Commander::handle_update_bandwidthKHz() {
  // Implementation for updating bandwidth
  ESP_LOGD(TAG, "Update bandwidth command executing");

  char* data = readAndRemove();  // Read and remove the command token

  // convert to integer
  if (data == nullptr) {
    ESP_LOGW(TAG, "Empty data received for gain update, expecting <float>");
    return;
  }

  float bandwidthKhz = static_cast<float>(atof(data));  // Cast to float
  m_loraCom->setBandwidth(bandwidthKhz);  // Set the gain in LoRaCom
}
#ifdef SFTU
void Commander::handle_set_OUTPUT() {
  ESP_LOGD(TAG, "Set output command executing");
  char* data = readAndRemove();  // Read and remove the command token

  // convert to integer
  if (data == nullptr) {
    ESP_LOGW(TAG, "Empty data received for output set, expecting <uint8_t>");
    return;
  }

  bool state = ((atoi(data)) == 1) ? true : false;
  ESP_LOGI(TAG, "Setting output to %s", state ? "ON" : "OFF");
}
#else
void Commander::handle_set_OUTPUT() {
  ESP_LOGD(TAG, "Command not implemented for this build");
}
#endif

void Commander::handle_mode() {
  // Implementation for mode command
  ESP_LOGD(TAG, "Mode command not implemented yet");
}

// ----- Get Handlers Implementation -----
void Commander::handle_get_help() {
  handle_help(get_handler);
}

void Commander::handle_get_gain() {
  if (m_control && m_control->getLoRaCom()) {
    // LoRaCom doesn't have getGain method, use a default response
    ESP_LOGI(TAG, "Gain query not implemented in LoRaCom");
    if (m_serialCom) {
      m_serialCom->sendData("gain not_available\n");
    }
  }
}

void Commander::handle_get_freq() {
  if (m_control && m_control->getLoRaCom()) {
    // LoRaCom doesn't have getFrequency method, use a default response
    ESP_LOGI(TAG, "Frequency query not implemented in LoRaCom");
    if (m_serialCom) {
      String response = String("freq ") + String(LORA_FREQUENCY, 3) + "\n";
      m_serialCom->sendData(response.c_str());
    }
  }
}

void Commander::handle_get_sf() {
  if (m_control && m_control->getLoRaCom()) {
    // LoRaCom doesn't have getSpreadingFactor method, use a default response
    ESP_LOGI(TAG, "Spreading factor query not implemented in LoRaCom");
    if (m_serialCom) {
      m_serialCom->sendData("sf not_available\n");
    }
  }
}

void Commander::handle_get_bw() {
  if (m_control && m_control->getLoRaCom()) {
    // LoRaCom doesn't have getBandwidth method, use a default response
    ESP_LOGI(TAG, "Bandwidth query not implemented in LoRaCom");
    if (m_serialCom) {
      m_serialCom->sendData("bw not_available\n");
    }
  }
}

void Commander::handle_get_status() {
  if (m_control) {
    bool status = m_control->isStatusEnabled();
    ESP_LOGI(TAG, "LoRa status enabled: %d", status);
    if (m_serialCom) {
      String response = String("status ") + String(status ? "1" : "0") + "\n";
      m_serialCom->sendData(response.c_str());
    }
  }
}

void Commander::handle_get_interval() {
  // This would need to be implemented in the control class
  ESP_LOGI(TAG, "Get interval not implemented yet");
  if (m_serialCom) {
    m_serialCom->sendData("interval 30\n");  // Default value
  }
}

void Commander::handle_get_wifi_en() {
  if (m_control) {
    bool wifi_en = m_control->isWiFiEnabled();
    ESP_LOGI(TAG, "WiFi enabled: %d", wifi_en);
    if (m_serialCom) {
      String response = String("wifi_en ") + String(wifi_en ? "1" : "0") + "\n";
      m_serialCom->sendData(response.c_str());
    }
  }
}

void Commander::handle_get_post_mode() {
  if (m_control) {
    bool post_mode = m_control->getPostOnLora();
    ESP_LOGI(TAG, "POST mode: %s", post_mode ? "lora" : "time");
    if (m_serialCom) {
      String response = String("post_mode ") + String(post_mode ? "lora" : "time") + "\n";
      m_serialCom->sendData(response.c_str());
    }
  }
}

void Commander::handle_get_post_en() {
  if (m_control) {
    bool post_en = m_control->isPostEnabled();
    ESP_LOGI(TAG, "POST enabled: %d", post_en);
    if (m_serialCom) {
      String response = String("post_en ") + String(post_en ? "1" : "0") + "\n";
      m_serialCom->sendData(response.c_str());
    }
  }
}

void Commander::handle_get_wifi_status() {
  if (m_control) {
    bool connected = m_control->isWiFiConnected();
    ESP_LOGI(TAG, "WiFi connected: %d", connected);
    if (m_serialCom) {
      String response = String("wifi_status ") + String(connected ? "1" : "0") + "\n";
      m_serialCom->sendData(response.c_str());
    }
  }
}

void Commander::handle_get_http_status() {
  if (m_control) {
    String http_status = m_control->getLastHttpResult();
    ESP_LOGI(TAG, "HTTP status: %s", http_status.c_str());
    if (m_serialCom) {
      String response = String("http_status ") + http_status + "\n";
      m_serialCom->sendData(response.c_str());
    }
  }
}

void Commander::handle_get_ssid() {
  ESP_LOGI(TAG, "handle_get_ssid called");
  if (m_control) {
    String ssid = m_control->getWiFiSSID();
    ESP_LOGI(TAG, "Current SSID: %s", ssid.c_str());
    if (m_serialCom) {
      String response = String("ssid ") + ssid + "\n";
      m_serialCom->sendData(response.c_str());
      ESP_LOGI(TAG, "Sent response: %s", response.c_str());
    } else {
      ESP_LOGE(TAG, "m_serialCom is null");
    }
  } else {
    ESP_LOGE(TAG, "m_control is null");
  }
}

void Commander::handle_get_password() {
  ESP_LOGI(TAG, "handle_get_password called");
  if (m_control) {
    String password = m_control->getWiFiPassword();
    ESP_LOGI(TAG, "Current password: %s", password.length() > 0 ? "****" : "(empty)");
    if (m_serialCom) {
      String response = String("password ") + password + "\n";
      m_serialCom->sendData(response.c_str());
      ESP_LOGI(TAG, "Sent response: %s", response.c_str());
    } else {
      ESP_LOGE(TAG, "m_serialCom is null");
    }
  } else {
    ESP_LOGE(TAG, "m_control is null");
  }
}

// ----- WiFi Handlers Implementation -----
void Commander::handle_set_wifi_credentials() {
  ESP_LOGD(TAG, "Set WiFi credentials command executing");

  char* ssid_data = readAndRemove();
  char* password_data = readAndRemove();

  if (ssid_data == nullptr || password_data == nullptr) {
    ESP_LOGW(TAG, "Missing SSID or password for WiFi credentials");
    return;
  }

  String new_ssid = String(ssid_data);
  String new_password = String(password_data);

  if (m_control) {
    m_control->setWiFiCredentials(new_ssid, new_password);
    ESP_LOGI(TAG, "WiFi credentials updated: SSID=%s", new_ssid.c_str());
  }
}

void Commander::checkCommand(const HandlerMap* handler_) {
  char* token = readAndRemove();
  if (token != nullptr) {
    runMappedCommand(token, handler_);  // Run the mapped command
  } else {
    ESP_LOGW(TAG,
             "No command provided, type <help> after action for a list of "
             "commands. eg: <command help>, <command update help>, etc.");
  }
}

void Commander::runMappedCommand(char* command, const HandlerMap* handler) {
  ESP_LOGI(TAG, "Looking for command: '%s'", command);
  // Call the mapped command handler
  for (const HandlerMap* cmd = handler; cmd->name != nullptr; ++cmd) {
    ESP_LOGD(TAG, "Checking against: '%s'", cmd->name);
    if (c_cmp(command, cmd->name)) {
      ESP_LOGI(TAG, "Found matching command: '%s'", cmd->name);
      return (this->*cmd->handler)();  // Call the corresponding handler
    }
  }
  ESP_LOGW(TAG, "Command '%s' not found in handler map", command);
}

char* Commander::readAndRemove() {
  if (m_command == nullptr || *m_command == nullptr) return nullptr;

  char* start = *m_command;

  // Skip leading spaces
  while (*start == ' ') start++;

  // If we reached the end, return nullptr
  if (*start == '\0') {
    *m_command = nullptr;
    return nullptr;
  }

  // Find the end of the current token
  char* end = start;
  while (*end != ' ' && *end != '\0') end++;

  // If we found a space, null-terminate the token and update buffer
  if (*end == ' ') {
    *end = '\0';           // Null-terminate the current token
    *m_command = end + 1;  // Point to the rest of the string
  } else {
    // No more tokens after this one
    *m_command = nullptr;
  }

  return start;
}

void Commander::setCommand(const char* buffer) {
  if (buffer != nullptr) {
    *m_command = strdup(buffer);
    ESP_LOGD(TAG, "Command set: %s", *m_command);
  } else {
    ESP_LOGW(TAG, "Attempted to set a null buffer");
  }
}

// Definitions for static members
const Commander::HandlerMap Commander::command_handler[6] = {
    {"help", &Commander::handle_command_help},
    {"update", &Commander::handle_update},
    {"set", &Commander::handle_set},
    {"get", &Commander::handle_get},
    {"mode", &Commander::handle_mode},
    {nullptr, nullptr}};

const Commander::HandlerMap Commander::update_handler[6] = {
    {"help", &Commander::handle_update_help},
    {"gain", &Commander::handle_update_gain},
    {"freqMhz", &Commander::handle_update_freqMhz},
    {"sf", &Commander::handle_update_spreadingFactor},
    {"bwKHz", &Commander::handle_update_bandwidthKHz},
    {nullptr, nullptr}};

const Commander::HandlerMap Commander::set_handler[4] = {
    {"help", &Commander::handle_set_help},
    {"output", &Commander::handle_set_OUTPUT},
    {"wifi_credentials", &Commander::handle_set_wifi_credentials},
    {nullptr, nullptr}};

const Commander::HandlerMap Commander::get_handler[15] = {
    {"help", &Commander::handle_get_help},
    {"gain", &Commander::handle_get_gain},
    {"freq", &Commander::handle_get_freq},
    {"sf", &Commander::handle_get_sf},
    {"bw", &Commander::handle_get_bw},
    {"status", &Commander::handle_get_status},
    {"interval", &Commander::handle_get_interval},
    {"wifi_en", &Commander::handle_get_wifi_en},
    {"post_mode", &Commander::handle_get_post_mode},
    {"post_en", &Commander::handle_get_post_en},
    {"wifi_status", &Commander::handle_get_wifi_status},
    {"http_status", &Commander::handle_get_http_status},
    {"ssid", &Commander::handle_get_ssid},
    {"password", &Commander::handle_get_password},
    {nullptr, nullptr}};
