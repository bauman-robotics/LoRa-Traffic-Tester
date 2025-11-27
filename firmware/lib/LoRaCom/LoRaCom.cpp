// LoRaCom.cpp
#include "LoRaCom.hpp"
#include "../lora_config.hpp"
#include "../wifi_manager/wifi_manager.hpp"

// Meshtastic-style duty cycle parameters
#define MESHTASTIC_PREAMBLE_LENGTH 20 // было 8 - не правильно. 
#define MESHTASTIC_RADIOLIB_IRQ_RX_FLAGS RADIOLIB_IRQ_RX_DONE | RADIOLIB_IRQ_PREAMBLE_DETECTED | RADIOLIB_IRQ_HEADER_VALID

extern void* wifi_manager_global;
extern volatile bool force_lora_trigger;

LoRaCom *LoRaCom::instance = nullptr;

LoRaCom::LoRaCom() {
  instance = this;  // Set the static instance pointer
  ESP_LOGI(TAG, "LoRaCom constructor called");
}

// void LoRaCom::setRxFlag() {
//   if (instance) {
//     instance->RxFlag = true;
//   }
// }

void LoRaCom::RxTxCallback(void) {
  if (instance) {
    if (instance->TxMode) {
      // Just set flag, actual processing in task
      instance->TxFinished = true;
      return;
    }
    instance->RxFlag = true;
  }
}

void LoRaCom::sendMessage(const char *msg) {
  if (isFakeMode) {
    if (msg[0] != '\0') {
      ESP_LOGI(TAG, "Fake transmitting: <%s>", msg);
    }
  } else {
    if (radioInitialised && !TxMode) {
      // Clear any pending RxFlag to allow transmission
      RxFlag = false;
      if (msg[0] != '\0') {
        size_t msgLen = strlen(msg);
        if (msgLen > 64) {  // Limit message length to prevent issues
          ESP_LOGE(TAG, "Message too long: %d bytes, max 64", msgLen);
          return;
        }
        int state = radioUnion.sRadio->startTransmit(msg);
        instance->TxMode = true;
        instance->txStartTime = millis();  // Record transmission start time
        if (state == RADIOLIB_ERR_NONE) {
          ESP_LOGD(TAG, "Transmitting: <%s>", msg);
        } else {
          ESP_LOGE(TAG, "Failed to begin transmission, code: %d", state);
          instance->TxMode = false;  // Reset flag on failure
        }
      }
    } else {
      ESP_LOGW(TAG, "Cannot send: radioInitialised=%d, TxMode=%d",
               radioInitialised, TxMode);
    }
  }
}

bool LoRaCom::checkTxMode() {
  return TxMode;  // Return the current transmission mode status
}

bool LoRaCom::getMessage(char *buffer, size_t len, int* receivedLen) {
  int actualLen = 0;
  if (isFakeMode) {
    // Simulate receiving a message occasionally
    static int counter = 0;
    counter++;
    if (counter % 1000 == 0) {  // Every 1000th call - reduce spam
      const char *fakeMsg = "Fake received message";
      if (strlen(fakeMsg) < len) {
        strcpy((char*)buffer, fakeMsg);
        actualLen = strlen(fakeMsg);
        if (receivedLen) *receivedLen = actualLen;
        return true;
      }
    }
    return false;
  } else {
    // Check if transmission finished
    if (TxFinished && radioInitialised) {
      TxFinished = false;
      unsigned long txDuration = millis() - txStartTime;  // Calculate transmission duration
      int state = radioUnion.sRadio->finishTransmit();
#if DUTY_CYCLE_RECEPTION == 1
      // Use Meshtastic-style duty cycle reception for power efficiency
      ESP_LOGD(TAG, "Using duty cycle reception after TX");
      state |= radioUnion.sRadio->startReceiveDutyCycleAuto(MESHTASTIC_PREAMBLE_LENGTH, 8, MESHTASTIC_RADIOLIB_IRQ_RX_FLAGS);
#else
      // Use continuous receive for traffic testing
      ESP_LOGD(TAG, "Using continuous reception after TX");
      state |= radioUnion.sRadio->startReceive();
#endif
      TxMode = false;
      if (state == RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Tx done: %lu ms SF%d BW%.0f", txDuration, currentSF, currentBW);
      } else {
        ESP_LOGE(TAG, "Transmission failed, code: %d", state);
      }
    }

    if (RxFlag && radioInitialised) {
      int state;  // Объявляем переменную state в начале блока

#if PARSE_SENDER_ID_FROM_LORA_PACKETS == 1
      // NEW METHOD: Parse sender_id from packet header when enabled
      size_t packetLength = radioUnion.sRadio->getPacketLength();
      if (packetLength > 7) {
        // Read the entire packet including header
        uint8_t tempBuffer[packetLength];
        state = radioUnion.sRadio->readData(tempBuffer, packetLength);

        // Extract destination_id and sender_id from header (Meshtastic format)
        // Header format: to(4), from(4), id(4), channel/flags(1+) ...
        if (packetLength >= 8) {
          uint32_t destinationId = (tempBuffer[3] << 24) | (tempBuffer[2] << 16) | (tempBuffer[1] << 8) | tempBuffer[0];          
          ((WiFiManager*)wifi_manager_global)->setLastDestinationId(destinationId);

          uint32_t lastSenderId = (tempBuffer[7] << 24) | (tempBuffer[6] << 16) | (tempBuffer[5] << 8) | tempBuffer[4];
          ((WiFiManager*)wifi_manager_global)->setLastSenderId(lastSenderId);
          
          ESP_LOGI(TAG, "Parsed destination_id: %u, sender_id: %u from packet length %d", destinationId, lastSenderId, packetLength);
        }

        // Copy payload to user's buffer (skip header if needed)
        //size_t payloadLen = packetLength - 8;  // Assume 8-byte header
        size_t payloadLen = packetLength;  // Assume 8-byte header
        if (payloadLen > len) payloadLen = len;
        memcpy((uint8_t*)buffer, tempBuffer + 8, payloadLen);
        actualLen = payloadLen;
      } else {
        // Short packet - use sender_id = 1
        lastSenderId = 1;
        state = radioUnion.sRadio->readData(reinterpret_cast<uint8_t *>(buffer), len);
        // Use same length detection logic as before
        actualLen = 0;
        for (size_t i = 0; i < len; i++) {
          if (buffer[i] == '\0') {
            actualLen = i;
            break;
          }
        }
        if (actualLen == 0) actualLen = len;
        ESP_LOGI(TAG, "Short packet received, using sender_id = 1");
      }
#else
      // OLD METHOD: Original logic when parsing is disabled - no getPacketLength() call
      state = radioUnion.sRadio->readData(reinterpret_cast<uint8_t *>(buffer), len);
#endif

      RxFlag = false;
      state |= radioUnion.sRadio->startReceive();
      bool result = (state == RADIOLIB_ERR_NONE);

      if (result) {
        // Determine actual packet length
#if PARSE_SENDER_ID_FROM_LORA_PACKETS == 0
        // Original length detection when parsing disabled
        actualLen = 0;
        for (size_t i = 0; i < len; i++) {
          if (buffer[i] == '\0') {
            actualLen = i;
            break;
          }
        }
        if (actualLen == 0) {  // No null terminator found, assume we got data
          // This is tricky - RadioLib doesn't return actual received length
          // We'll use a heuristic or assume received something meaningful
          actualLen = len;  // Default assumption
        }
#endif

        if (receivedLen) *receivedLen = actualLen;
      }
      if (result && (POST_EN_WHEN_LORA_RECEIVED || force_lora_trigger) && wifi_manager_global) {
        ESP_LOGI(TAG, "LoRa packet received, sending POST trigger");
        ((WiFiManager*)wifi_manager_global)->setLoRaRssi(abs(radioUnion.sRadio->getRSSI()));
        ((WiFiManager*)wifi_manager_global)->setLastFullPacketLen(packetLength);  // Set full packet length for Flask server
        ((WiFiManager*)wifi_manager_global)->setSendPostOnLoRa(true);
      }
      return result;
    }
    return false;
  }
}

int32_t LoRaCom::getRssi() {
  if (isFakeMode) {
    return -40;  // Fake RSSI
  } else {
    return radioUnion.sRadio->getRSSI();  // Return the last received signal strength
  }
}

/* ================================ SETTERS ================================ */

bool LoRaCom::setOutGain(int8_t gain) {
  if (isFakeMode) {
    ESP_LOGI(TAG, "Fake gain set to %d", gain);
    return true;
  } else {
    // value should be bewteen -9 and 22 dBm
    int state = radioUnion.sRadio->setOutputPower(gain);
    if (state == RADIOLIB_ERR_NONE) {
      ESP_LOGI(TAG, "Gain set to %d", gain);
      return true;
    } else {
      ESP_LOGE(TAG, "Failed to set gain with code: %d", state);
      return false;
    }
  }
}

bool LoRaCom::setFrequency(float freqMHz) {
  if (isFakeMode) {
    ESP_LOGI(TAG, "Fake frequency set to %.2f MHz", freqMHz);
    return true;
  } else {
    // Set the frequency of the radio
    int state = radioUnion.sRadio->setFrequency(freqMHz);
    if (state == RADIOLIB_ERR_NONE) {
      ESP_LOGI(TAG, "Frequency set to %.2f MHz", freqMHz);
      return true;
    } else {
      ESP_LOGE(TAG, "Failed to set frequency with code: %d", state);
      return false;
    }
  }
}

bool LoRaCom::setSpreadingFactor(uint8_t spreadingFactor) {
  if (isFakeMode) {
    ESP_LOGI(TAG, "Fake spreading factor set to %d", spreadingFactor);
    currentSF = spreadingFactor;
    return true;
  } else {
    // Set the spreading factor of the radio
    int state = radioUnion.sRadio->setSpreadingFactor(spreadingFactor);
    if (state == RADIOLIB_ERR_NONE) {
      ESP_LOGI(TAG, "Spreading factor set to %d", spreadingFactor);
      currentSF = spreadingFactor;
      // Force radio reconfiguration for new parameters to take effect
      radioUnion.sRadio->startReceive();
      return true;
    } else {
      ESP_LOGE(TAG, "Failed to set spreading factor with code: %d", state);
      return false;
    }
  }
}

bool LoRaCom::setBandwidth(float bandwidth) {
  if (isFakeMode) {
    ESP_LOGI(TAG, "Fake bandwidth set to %.2f kHz", bandwidth);
    currentBW = bandwidth;
    return true;
  } else {
    // Set the bandwidth of the radio
    int state = radioUnion.sRadio->setBandwidth(bandwidth);
    if (state == RADIOLIB_ERR_NONE) {
      ESP_LOGI(TAG, "Bandwidth set to %.2f kHz", bandwidth);
      currentBW = bandwidth;
      // Force radio reconfiguration for new parameters to take effect
      radioUnion.sRadio->startReceive();
      return true;
    } else {
      ESP_LOGE(TAG, "Failed to set bandwidth with code: %d", state);
      return false;
    }
  }
}
