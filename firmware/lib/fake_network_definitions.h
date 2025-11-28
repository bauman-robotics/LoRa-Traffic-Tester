#pragma once

#include <Arduino.h>

// Compile-time network settings
// Edit these values directly in this file
//

// HTTPS security settings
#define USE_INSECURE_HTTPS 1  // Если 1, пропустить проверку SSL сертификатов (эквивалент curl -k); если 0, проверять сертификаты

const char* const DEFAULT_WIFI_SSID = "fake_ssid";
const char* const DEFAULT_WIFI_PASSWORD = "fake_pass";

const char* const DEFAULT_API_KEY = "fake_key";
const char* const DEFAULT_USER_ID = "fake_user";
const char* const DEFAULT_USER_LOCATION = "fake_loc";
#ifdef USE_HTTPS
const char* const DEFAULT_SERVER_PROTOCOL = "https";
#else
const char* const DEFAULT_SERVER_PROTOCOL = "http";
#endif
const char* const DEFAULT_SERVER_IP = "192.168.1.1";
const char* const DEFAULT_SERVER_PATH = "fake_path.php";

// Flask server settings (alternative to PHP server)
const char* const DEFAULT_FLASK_API_KEY = "flask_api_key";
const char* const DEFAULT_FLASK_USER_ID = "new_device_001";
const char* const DEFAULT_FLASK_USER_LOCATION = "location_after_clear";
#ifdef USE_HTTPS
const char* const DEFAULT_FLASK_SERVER_PROTOCOL = "https";
#else
const char* const DEFAULT_FLASK_SERVER_PROTOCOL = "http";
#endif
const char* const DEFAULT_FLASK_SERVER_IP = "192.168.1.1";
#ifdef USE_HTTPS
const char* const DEFAULT_FLASK_SERVER_PORT = "443";
#else
const char* const DEFAULT_FLASK_SERVER_PORT = "5001";
#endif
const char* const DEFAULT_FLASK_SERVER_PATH = "/api/lora";
