#pragma once

#include <WiFi.h>
#include <WiFiClient.h>
#include <Arduino.h>
#include "../wifi_test_config.hpp"

class WiFiManagerSimple {
public:
    WiFiManagerSimple();
    ~WiFiManagerSimple();

    bool connect();
    void disconnect();
    bool isConnected();
    bool doHttpPost();

    // Setters for hot/cold values if needed to change at runtime
    void setHotValue(int hot) { hot_value = hot; }
    void setColdValue(int cold) { cold_value = cold; }

    // Getters
    int getHotValue() { return hot_value; }
    int getColdValue() { return cold_value; }

private:
    String ssid;
    String password;
    String apiKey;
    String userId;
    String userLocation;
    String serverProtocol;
    String serverIP;
    String serverPath;
    String serverPort;

    int hot_value;
    int cold_value;

    // Statistics
    unsigned long postsSent;
    unsigned long lastPostTime;
    unsigned long connectionTime;
    unsigned long lastSuccessfulPost;
    bool isCurrentlyConnected;

#if WIFI_DEBUG_FIXES
    static void WiFiEvent(WiFiEvent_t event);
#endif
};
