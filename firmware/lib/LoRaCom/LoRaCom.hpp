// LoRaCom.h
#ifndef LoRaCom_h
#define LoRaCom_h

#include <Arduino.h>
#include <RadioLib.h>

#include "esp_log.h"

// Forward declaration for fake mode - always available for fallback
class FakeRadio {
 public:
  int begin(float freq, float bw, uint8_t sf, uint8_t cr, uint8_t syncWord, int8_t power, uint16_t preambleLength, float tcxoVoltage = 1.6, bool useRegulatorLDO = false) { return 0; }
  int startTransmit(const char *msg) { return 0; }
  int finishTransmit() { return 0; }
  int startReceive() { return 0; }
  int readData(uint8_t *buffer, size_t len) { return 0; }
  int32_t getRSSI() { return -40; }
  int setOutputPower(int8_t power) { return 0; }
  int setFrequency(float freq) { return 0; }
  int setSpreadingFactor(uint8_t sf) { return 0; }
  int setBandwidth(float bw) { return 0; }
  int setDio2AsRfSwitch(bool enable) { return 0; }
  void setPacketReceivedAction(void (*func)(void)) {}
};

class LoRaCom {
 public:
  LoRaCom();



  bool beginFake(uint8_t CLK, uint8_t MISO, uint8_t MOSI, uint8_t csPin,
                 uint8_t intPin, uint8_t RST, float freqMHz, int8_t power,
                 int8_t BUSY = -1) {
    radioUnion.fRadio = new FakeRadio();
    int state = radioUnion.fRadio->begin(freqMHz, 500, 7, 5, 0x34, power, 20);
    if (state == 0) {
      ESP_LOGI(TAG, "***FAKE LoRa MODE ENABLED - NO HARDWARE DETECTED***");
      radioInitialised = true;
      isFakeMode = true;
      return true;
    } else {
      ESP_LOGE(TAG, "Fake LoRa initialisation FAILED!");
      return false;
    }
  }

  template <typename RadioType>
  bool begin(uint8_t CLK, uint8_t MISO, uint8_t MOSI, uint8_t csPin,
             uint8_t intPin, uint8_t RST, float freqMHz, int8_t power,
             int8_t BUSY = -1) {
    SPI.begin(CLK, MISO, MOSI, csPin);

    radioUnion.sRadio = new RadioType((BUSY == -1) ? new Module(csPin, intPin, RST)
                                                   : new Module(csPin, intPin, RST, BUSY));

#if defined(SX126X_DIO3_TCXO_VOLTAGE)
    // SX1262::begin(freq, bw, sf, cr, syncWord, power, preambleLength, tcxoVoltage)
    // freq: 868.06 MHz, bw: 125 kHz, sf: 11, cr: 4/5, syncWord: 0x12, power: 22 dBm, preamble: 20, tcxo: 1.8V
    int state = radioUnion.sRadio->begin(freqMHz, 250.0, 11, 5, 0x12,
                                         power, 20, SX126X_DIO3_TCXO_VOLTAGE);
#else
    // SX1262::begin(freq, bw, sf, cr, syncWord, power, preambleLength)
    // freq: 868.075 MHz, bw: 250 kHz, sf: 11, cr: 4/5, syncWord: 0x12, power: 20 dBm, preamble: 20
    int state = radioUnion.sRadio->begin(freqMHz, 250.0, 11, 5, 0x12,
                                         power, 20);
#endif

#if defined(SX126X_DIO2_AS_RF_SWITCH)
    // Set DIO2 as RF switch for antenna multiplexing
    state |= radioUnion.sRadio->setDio2AsRfSwitch(true);
#endif

    radioUnion.sRadio->setPacketReceivedAction(RxTxCallback);
    // radio->setPacketSentAction(TxCallback);

    state |= radioUnion.sRadio->startReceive();
    if (state == RADIOLIB_ERR_NONE) {
      ESP_LOGI(TAG, "LoRa initialised successfully!");
      radioInitialised = true;
      isFakeMode = false;
      return true;
    } else {
      ESP_LOGE(TAG, "LoRa initialisation FAILED! Code: %d", state);
      return false;
    }
  }

  void sendMessage(const char *msg);  // overloaded function
  bool getMessage(char *buffer, size_t len, int* receivedLen = nullptr);
  int32_t getRssi();

  bool setOutGain(int8_t gain);
  bool setFrequency(float freqMHz);

  bool setSpreadingFactor(uint8_t spreadingFactor);
  bool setBandwidth(float bandwidth);

  void setCodingRate(int cr) { currentCR = cr; if (!isFakeMode) radioUnion.sRadio->setCodingRate(cr); }
  void setSyncWord(uint8_t sw) { currentSyncWord = sw; if (!isFakeMode) radioUnion.sRadio->setSyncWord(sw); }

  bool checkTxMode();

  uint8_t getCurrentSF() { return currentSF; }
  float getCurrentBW() { return currentBW; }
  int getCurrentCR() { return currentCR; }
  uint8_t getSyncWord() { return currentSyncWord; }

 private:
  union RadioUnion {
    FakeRadio* fRadio;
    SX1262* sRadio;
  } radioUnion;
  bool isFakeMode = false; // true if using FakeRadio, false if SX1262
  static LoRaCom *instance;

  bool radioInitialised = false;

  volatile bool RxFlag = false;

  volatile bool TxMode = false;

  volatile bool TxFinished = false;

  unsigned long txStartTime = 0;

  // Current LoRa settings for logging
  uint8_t currentSF = 11;
  float currentBW = 250.0;
  int currentCR = 5;
  uint8_t currentSyncWord = 0x12;

  // Last sender ID parsed from received packet
  uint32_t lastSenderId = 0;

  static void RxTxCallback(void);

  static constexpr const char *TAG = "";
};

#endif
