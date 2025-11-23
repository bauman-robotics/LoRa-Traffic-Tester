#pragma once

#include <Arduino.h>

// Compile-time network settings
// Edit these values directly in this file
//

const char* const DEFAULT_WIFI_SSID = "fake_ssid";
const char* const DEFAULT_WIFI_PASSWORD = "fake_pass";

const char* const DEFAULT_API_KEY = "fake_key";
const char* const DEFAULT_USER_ID = "fake_user";
const char* const DEFAULT_USER_LOCATION = "fake_loc";
const char* const DEFAULT_SERVER_PROTOCOL = "http";
const char* const DEFAULT_SERVER_IP = "192.168.1.1";
const char* const DEFAULT_SERVER_PATH = "fake_path.php";

// Flask server settings (alternative to PHP server)
const char* const DEFAULT_FLASK_API_KEY = "flask_api_key";
const char* const DEFAULT_FLASK_USER_ID = "new_device_001";
const char* const DEFAULT_FLASK_USER_LOCATION = "location_after_clear";
const char* const DEFAULT_FLASK_SERVER_PROTOCOL = "http";
const char* const DEFAULT_FLASK_SERVER_IP = "192.168.1.1";
const char* const DEFAULT_FLASK_SERVER_PORT = "5001";
const char* const DEFAULT_FLASK_SERVER_PATH = "/api/lora";
