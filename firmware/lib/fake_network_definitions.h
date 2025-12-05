#pragma once

//#include <Arduino.h>
#include "lora_config.hpp"

// Compile-time network settings
// Edit these values directly in this file

const char* const DEFAULT_WIFI_SSID = "fake_ssid";
const char* const DEFAULT_WIFI_PASSWORD = "fake_pass";

const char* const DEFAULT_API_KEY = "fake_key";
const char* const DEFAULT_USER_ID = "fake_user";
const char* const DEFAULT_USER_LOCATION = "fake_loc";
#if USE_HTTPS
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
#if USE_HTTPS
const char* const DEFAULT_FLASK_SERVER_PROTOCOL = "https";
#else
const char* const DEFAULT_FLASK_SERVER_PROTOCOL = "http";
#endif
const char* const DEFAULT_FLASK_SERVER_IP = "192.168.1.1";
#if USE_HTTPS
const char* const DEFAULT_FLASK_SERVER_PORT = "443";
#else
////// const char* const DEFAULT_FLASK_SERVER_PORT = "5001";  старый вариант, без nginx 
const char* const DEFAULT_FLASK_SERVER_PORT = "80";
#endif
const char* const DEFAULT_FLASK_SERVER_PATH = "/api/lora";
