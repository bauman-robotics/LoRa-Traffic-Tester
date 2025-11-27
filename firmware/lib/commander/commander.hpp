#pragma once

#include <Arduino.h>

#include <cstring>

#include "LoRaCom.hpp"
#include "SerialCom.hpp"

class Control;  // Forward declaration

#define c_cmp(a, b) (strcmp(a, b) == 0)

class Commander {
 public:
  Commander(SerialCom *serialCom, LoRaCom *loraCom, Control *control = nullptr);

 private:
  char *m_command[128];  // Buffer to store the command

  uint16_t m_timeout = 20000;  // 20 second timeout for commands

  SerialCom *m_serialCom;  // Pointer to SerialCom instance
  LoRaCom *m_loraCom;      // Pointer to LoRaCom instance
  Control *m_control;       // Pointer to Control instance

  typedef void (Commander::*Handler)();

  struct HandlerMap {
    const char *name;
    Handler handler;
  };

  // ----- Command Handlers -----
  void handle_command_help();  // Command handler for "help"
  void handle_update();        // Command handler for "update" parameters
  void handle_set();           // Command handler for "set" parameters
  void handle_get();           // Command handler for "get" parameters
  void handle_mode();  // Command handler for "mode" (eg: transmit, receive,
                       // transcieve, spectrum scan, etc")

  // ----- Update Handlers -----
  void handle_update_help();             // Command handler for "help"
  void handle_update_gain();             // Command handler for "update gain"
  void handle_update_freqMhz();          // Command handler for "update freqMhz"
  void handle_update_spreadingFactor();  // Command handler for "update
                                         // spreading factor"
  void handle_update_bandwidthKHz();  // Command handler for "update bandwidth"

  void handle_set_help();
  void handle_set_OUTPUT();

  // ----- Get Handlers -----
  void handle_get_help();
  void handle_get_gain();
  void handle_get_freq();
  void handle_get_sf();
  void handle_get_bw();
  void handle_get_status();
  void handle_get_interval();
  void handle_get_wifi_en();
  void handle_get_post_mode();
  void handle_get_post_en();
  void handle_get_wifi_status();
  void handle_get_http_status();
  void handle_get_ssid();
  void handle_get_password();

  // ----- WiFi Handlers -----
  void handle_set_wifi_credentials();

  void handle_help(const HandlerMap *handler);

  static const HandlerMap command_handler[6];
  static const HandlerMap update_handler[6];
  static const HandlerMap set_handler[4];
  static const HandlerMap get_handler[15];

  void runMappedCommand(char *command, const HandlerMap *handler);

  static constexpr const char *TAG = "Commander";

 public:
  void checkCommand(const HandlerMap *handler =
                        command_handler);  // Check the command and run
                                           // the appropriate handler

  void setCommand(const char *buffer);

  char *readAndRemove();
};
