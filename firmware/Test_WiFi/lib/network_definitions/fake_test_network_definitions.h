// Network definitions for Test_WiFi project
// Copy your main project network settings here

#define DEFAULT_WIFI_SSID "Your_WIFI_SSID"
#define DEFAULT_WIFI_PASSWORD "Your_WIFI_PASSWORD"
#define DEFAULT_API_KEY "your_api_key_here"
#define DEFAULT_USER_ID "test_device"
#define DEFAULT_USER_LOCATION "test_location"
#define DEFAULT_SERVER_PROTOCOL "http"
#define DEFAULT_SERVER_IP "192.168.1.100"  // Change to your server IP
#define DEFAULT_SERVER_PORT "5001" 

#define DEFAULT_SERVER_PATH "api/data"

// WiFi connection settings
#define WIFI_CONNECT_ATTEMPTS 3
#define WIFI_CONNECT_INTERVAL_MS 2000
#define WIFI_CONNECT_TIMEOUT_MS 15000

// POST settings
#define POST_HOT_DEFAULT 0
#define POST_COLD_DEFAULT 0
#define POST_INTERVAL_MS 10000  // 10 seconds
#define ALARM_TIME 100

// Buffer sizes
#define POST_DATA_BUFFER_SIZE 512
#define HTTP_RESPONSE_BUFFER_SIZE 1024
