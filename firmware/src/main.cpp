#include <Arduino.h>

#include "control.hpp"

Control* control = nullptr;

void setup() {
  delay(1000);
  ESP_LOGI("Main", "Starting setup...");
  ESP_LOGI("Main", "DUTY_CYCLE_RECEPTION=%d (0=continuous receive, 1=duty cycle like Meshtastic)", DUTY_CYCLE_RECEPTION);
  control = new Control();
  control->setup();
  control->begin();
}

void loop() {
  vTaskDelay(pdMS_TO_TICKS(10000));  // random delay to allow tasks to run
}

/*
  RadioLib SX126x Receive after Channel Activity Detection Example

  This example uses SX1262 to scan the current LoRa
  channel and detect ongoing LoRa transmissions.
  Unlike SX127x CAD, SX126x can detect any part
  of LoRa transmission, not just the preamble.
  If a packet is detected, the module will switch
  to receive mode and receive the packet.

  Other modules from SX126x family can also be used.

  For default module settings, see the wiki page
  https://github.com/jgromes/RadioLib/wiki/Default-configuration#sx126x---lora-modem

  For full API reference, see the GitHub Pages
  https://jgromes.github.io/RadioLib/
*/

