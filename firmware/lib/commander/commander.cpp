#include "commander.hpp"

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
const Commander::HandlerMap Commander::command_handler[5] = {
    {"help", &Commander::handle_command_help},
    {"update", &Commander::handle_update},
    {"set", &Commander::handle_set},
    {"mode", &Commander::handle_mode},
    {nullptr, nullptr}};

const Commander::HandlerMap Commander::update_handler[6] = {
    {"help", &Commander::handle_update_help},
    {"gain", &Commander::handle_update_gain},
    {"freqMhz", &Commander::handle_update_freqMhz},
    {"sf", &Commander::handle_update_spreadingFactor},
    {"bwKHz", &Commander::handle_update_bandwidthKHz},
    {nullptr, nullptr}};

const Commander::HandlerMap Commander::set_handler[3] = {
    {"help", &Commander::handle_set_help},
    {"output", &Commander::handle_set_OUTPUT},
    {nullptr, nullptr}};
