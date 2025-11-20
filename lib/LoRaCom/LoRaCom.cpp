// LoRaCom.cpp
#include "LoRaCom.hpp"
#include "../lora_config.hpp"
#include "../wifi_manager/wifi_manager.hpp"

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

bool LoRaCom::getMessage(char *buffer, size_t len) {
  if (isFakeMode) {
    // Simulate receiving a message occasionally
    static int counter = 0;
    counter++;
    if (counter % 1000 == 0) {  // Every 1000th call - reduce spam
      const char *fakeMsg = "Fake received message";
      if (strlen(fakeMsg) < len) {
        strcpy(buffer, fakeMsg);
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
      state |= radioUnion.sRadio->startReceive();
      TxMode = false;
      if (state == RADIOLIB_ERR_NONE) {
        ESP_LOGI(TAG, "Tx done: %lu ms SF%d BW%.0f", txDuration, currentSF, currentBW);
      } else {
        ESP_LOGE(TAG, "Transmission failed, code: %d", state);
      }
    }

    if (RxFlag && radioInitialised) {
      int state = radioUnion.sRadio->readData(reinterpret_cast<uint8_t *>(buffer), len);
      RxFlag = false;
      state |= radioUnion.sRadio->startReceive();
      bool result = (state == RADIOLIB_ERR_NONE);
    if (result && (POST_EN_WHEN_LORA_RECEIVED || force_lora_trigger) && wifi_manager_global) {
      ESP_LOGI(TAG, "LoRa packet received, sending POST trigger");
      ((WiFiManager*)wifi_manager_global)->setLoRaRssi(abs(radioUnion.sRadio->getRSSI()));
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
