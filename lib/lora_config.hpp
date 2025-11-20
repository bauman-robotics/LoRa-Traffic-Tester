#pragma once

// DIO2 подключен к антенному переключателю, и вам не нужно его подключать к MCU.
#define SX126X_DIO2_AS_RF_SWITCH
// DIO3 для TCXO 1.8V
#define SX126X_DIO3_TCXO_VOLTAGE (1.8)

#define LORA_RESET (5)
#define LORA_DIO1  (3)
#define LORA_BUSY  (4)

#define LORA_SCK  (10)
#define LORA_MISO (6)
#define LORA_MOSI (7)
#define LORA_CS   (8)

// LoRa default settings (compatible with E80 module)
#define LORA_FREQUENCY 869.075f
#define LORA_POWER 20

// Simulation mode: if 1, skip hardware LoRa initialization and use fake mode
#define FAKE_LORA 0

// Status sending interval (can be changed via commands)
extern unsigned long status_Interval;

// WiFi configuration
#define WIFI_ENABLE 1  // Enable WiFi functionality (runtime control via GUI)
#define WIFI_USE_CUSTOM_MAC 0  // Use custom random MAC address for WiFi (0=use ESP default, 1=generate random)
#define WIFI_USE_FIXED_MAC 0  // If 1, use fixed MAC address specified below
//#define WIFI_FIXED_MAC_ADDRESS {0x1c, 0xdb, 0xd4, 0xc6, 0x77, 0xf0}  // Fixed MAC address bytes (used if WIFI_USE_FIXED_MAC=1)
#define WIFI_FIXED_MAC_ADDRESS {0x1c, 0xdb, 0xd4, 0xC3, 0xC9, 0xD4} 
// MAC: 1C:DB:D4:C3:C9:D4   --- вариант, который заработал, без корпуса. 
#define WIFI_CONNECT_ATTEMPTS 5  // Number of WiFi connection attempts
#define WIFI_CONNECT_INTERVAL_MS 5000  // Interval between connection attempts
//#define WIFI_CONNECT_INTERVAL_MS 10000  // Test Interval between connection attempts
#define WIFI_DEBUG_FIXES 1  // Enable WiFi debug fixes and scan (set to 1 for ESP32-C3 supermini with issues)
#define WIFI_AUTO_TX_POWER_TEST 0    // Enable automatic testing of different TX power levels (0=use WIFI_TX_POWER_VARIANT, 1=auto test)
#define WIFI_ATTEMPTS_PER_VARIANT 2  // Number of attempts per TX power variant in auto test

// Network settings included from lib/network_definitions.h or fallback
// Edit lib/network_definitions.h to change settings at compile time
// If no main file, edit lib/fake_network_definitions.h instead

#define USE_SYSTEM_NETWORK 1

#ifdef USE_SYSTEM_NETWORK
#include "../../network_definitions.h"
#else
#include "fake_network_definitions.h"  // Fallback defaults
#endif

#define POST_INTERVAL_EN 0  // Enable periodic POST requests (can be controlled via GUI)
#define POST_EN_WHEN_LORA_RECEIVED 1  // Send POST only when LoRa packet received
#define POST_HOT_AS_RSSI 1  // If 1, use RSSI as hot parameter when POST triggered by LoRa receive; if 0, use successful POST count
#define SERVER_PING_ENABLED 0  // Enable periodic ping of the server

#define POST_INTERVAL_MS 10000  // Interval between POST requests in ms (if enabled)
#define PING_INTERVAL_MS 5000  // Interval between ping in ms
#define HOT_WATER 0  // Default value for hot water field
#define ALARM_TIME 200  // Alarm time field

// Initial value for cold water counter
#define COLD_INITIAL 1

// Periodic LoRa status sending settings
#define LORA_STATUS_ENABLED 0  // Enable/disable periodic LoRa status packets
#define LORA_STATUS_INTERVAL_SEC 10  // Default interval for LoRa status packets in seconds
#define LORA_STATUS_SHORT_PACKETS 1  // 0=full packet, 1=short 2-byte counter

// Mesh compatibility
#define MESH_COMPATIBLE 1  // If 1, set BW/SF/CR/sync to match Meshtastic SHORT_FAST for receiving their packets
#define MESH_SYNC_WORD 0x2B  // Private Meshtastic sync word
#define MESH_FREQUENCY 869.075f  // Frequency to use for Mesh compatibility
#define MESH_BANDWIDTH 250  // Bandwidth for Mesh (original narrow)
#define MESH_SPREADING_FACTOR 11  // SF for Mesh (matched to Meshtastic for trace packets)
#define MESH_CODING_RATE 5  // CR for Mesh

// Reception mode selection
#define DUTY_CYCLE_RECEPTION 1  // If 1, use Meshtastic-style duty cycle reception (energy efficient); if 0, use continuous receive (for traffic testing)

// If 1, send LoRa packet payload length as cold value in POST request (when POST_EN_WHEN_LORA_RECEIVED=1)
#define COLD_AS_LORA_PAYLOAD_LEN 1

// Counter for cold water (will be incremented)
extern unsigned long cold_counter;

#define WIFI_TX_POWER_VARIANT 7  // Variant for TX power testing (0-9 for different levels)
#if WIFI_TX_POWER_VARIANT == 0
#define WIFI_TX_POWER WIFI_POWER_20dBm  // 20 dBm (максимум для ESP32)
#elif WIFI_TX_POWER_VARIANT == 1
#define WIFI_TX_POWER WIFI_POWER_19_5dBm  // 19.5 dBm
#elif WIFI_TX_POWER_VARIANT == 2
#define WIFI_TX_POWER WIFI_POWER_19dBm  // 19 dBm
#elif WIFI_TX_POWER_VARIANT == 3
#define WIFI_TX_POWER WIFI_POWER_18_5dBm  // 18.5 dBm
#elif WIFI_TX_POWER_VARIANT == 4
#define WIFI_TX_POWER WIFI_POWER_18dBm  // 18 dBm
#elif WIFI_TX_POWER_VARIANT == 5
#define WIFI_TX_POWER WIFI_POWER_15dBm  // 15 dBm
#elif WIFI_TX_POWER_VARIANT == 6
#define WIFI_TX_POWER WIFI_POWER_10dBm  // 10 dBm
#elif WIFI_TX_POWER_VARIANT == 7
#define WIFI_TX_POWER WIFI_POWER_8_5dBm  // 8.5 dBm (низкая мощность для тестирования, сейчас используется)
#elif WIFI_TX_POWER_VARIANT == 8
#define WIFI_TX_POWER WIFI_POWER_2dBm  // 2 dBm (минимум)
#else
#define WIFI_TX_POWER WIFI_POWER_8_5dBm  // Default to 8.5dBm
#endif


//===  Не поднимался вайфай на esp32c3supermini. Что сработало ===
// Перенес WiFi.setTxPower((wifi_power_t)WIFI_TX_POWER) ПЕРЕД WiFi.begin() для исправления
// AUTH_EXPIRE на ESP32-C3 Super Mini. Также добавил логи "WiFi TX power set before begin" для диагностики.
