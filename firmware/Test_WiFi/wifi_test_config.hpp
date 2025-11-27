#pragma once

#include "lib/network_definitions/main_network_definitions.h"

// WiFi Configuration (copied from main lora_config.hpp)
#define WIFI_ENABLE 1
#define WIFI_USE_CUSTOM_MAC 0
#define WIFI_USE_FIXED_MAC 0
#define WIFI_FIXED_MAC_ADDRESS {0x1c, 0xdb, 0xd4, 0xC3, 0xC9, 0xD4}
#define WIFI_CONNECT_ATTEMPTS 3
#define WIFI_CONNECT_INTERVAL_MS 2000
#define WIFI_DEBUG_FIXES 1
#define WIFI_AUTO_TX_POWER_TEST 0
#define WIFI_ATTEMPTS_PER_VARIANT 2
#define WIFI_TX_POWER_VARIANT 7

// WiFi TX Power settings
#if WIFI_TX_POWER_VARIANT == 0
#define WIFI_TX_POWER WIFI_POWER_20dBm
#elif WIFI_TX_POWER_VARIANT == 1
#define WIFI_TX_POWER WIFI_POWER_19_5dBm
#elif WIFI_TX_POWER_VARIANT == 2
#define WIFI_TX_POWER WIFI_POWER_19dBm
#elif WIFI_TX_POWER_VARIANT == 3
#define WIFI_TX_POWER WIFI_POWER_18_5dBm
#elif WIFI_TX_POWER_VARIANT == 4
#define WIFI_TX_POWER WIFI_POWER_18dBm
#elif WIFI_TX_POWER_VARIANT == 5
#define WIFI_TX_POWER WIFI_POWER_15dBm
#elif WIFI_TX_POWER_VARIANT == 6
#define WIFI_TX_POWER WIFI_POWER_10dBm
#elif WIFI_TX_POWER_VARIANT == 7
#define WIFI_TX_POWER WIFI_POWER_8_5dBm
#elif WIFI_TX_POWER_VARIANT == 8
#define WIFI_TX_POWER WIFI_POWER_2dBm
#else
#define WIFI_TX_POWER WIFI_POWER_8_5dBm
#endif

// POST Configuration
#define POST_HOT_VALUE POST_HOT_DEFAULT    // Define your hot value here (0 by default)
#define POST_COLD_VALUE POST_COLD_DEFAULT  // Define your cold value here (0 by default)
#define POST_INTERVAL_MS 10000             // 10 seconds
#define SERVER_PING_ENABLED 0

// Logging
#define TAG "WiFiTest"
#define LOG_LEVEL_DEBUG 0  // Set to 1 for debug logs

// LED pin for connection status (optional)
#define STATUS_LED_PIN 2
