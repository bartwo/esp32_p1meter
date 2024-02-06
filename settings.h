// Uncomment for debugging
//#define DEBUG
// Uncomment for debug messages sent to gmail
//#define EMAIL_DEBUGGING
// Uncomment for debug messages sent to mqtt
//#define MQTT_DEBUGGING
// Uncomment for testing
//#define TEST

// Update treshold in milliseconds, messages will only be sent on this interval
//#define UPDATE_INTERVAL 1000 // 1 second
//#define UPDATE_INTERVAL 30000 // 30 seconds
#define UPDATE_INTERVAL 60000  // 1 minute
//#define UPDATE_INTERVAL 300000 // 5 minutes

// Update treshold in milliseconds,
// this will also send values that are more than the tresholds time the same
#define UPDATE_FULL_INTERVAL 600000 // 10 minutes
// #define UPDATE_FULL_INTERVAL 1800000 // 30 minutes
// #define UPDATE_FULL_INTERVAL 3600000 // 1 Hour

#define NA "N/A"

#define HOSTNAME "p1_meter"
#define OTA_PASSWORD_HASH "MD5 hash of my password"

#define BAUD_RATE 115200
#define RXD2 16
#define TXD2 17

//default
//#define LED_PIN LED_BUILTIN
#define LED_PIN 32

#define P1_MAXLINELENGTH 1050

#define MQTT_MAX_RECONNECT_TRIES 10
#define MQTT_ROOT_TOPIC "p1_meter/sensor"
#define MQTT_STATUS_TOPIC "p1_meter/status"
#ifdef MQTT_DEBUGGING
#define MQTT_DEBUG_TOPIC "p1_meter/debug"
#endif

#define WIFI_MAX_RECONNECT_TRIES 5

char WIFI_SSID[32] = "MyWifi";
char WIFI_PASS[32] = "MyWifiPassword";

char MQTT_HOST[64] = "homeassistant.local";
char MQTT_PORT[6] = "1883";
char MQTT_USER[32] = "mqtt";
char MQTT_PASS[32] = "mqttpass";

#ifdef EMAIL_DEBUGGING
char EMAIL_ADDRESS[32] = "name.surname@gmail.com";
char EMAIL_PASSWORD[32] = "mygmailappkey";
String emailMessageDump;
#endif

// if your P1 values with units need to be multiplied by any multiplier, specify it here
long VALUE_NUMERIC_MULTIPLIER = 1;

// Decimal places for float values, by default it's 2
// Integers are handled automatically and no decimals are used
unsigned short VALUE_FLOAT_DECIMAL_PLACES = 3;

// Specify CPU frequenxy in MHz to reduce power usage
// 0, if frequency should be handled automatically using ArduinoIDE and board settings
// CAUTION: please make sure your CPU can handle frequency!
unsigned int CPU_FREQ = 80;

// Nothing to change below - globals to make it all work as simple as possible
char telegram[P1_MAXLINELENGTH];

long LAST_UPDATE_SENT = 0;
long LAST_FULL_UPDATE_SENT = 0;

const char VALUE_START_CHAR = '(';
const char VALUE_END_CHAR = '*';
const char VALUE_NO_UNITS_END_CHAR = ')';

struct TelegramDecodedObject
{
  String name;
  String value;
  char code[16];
  bool sendData = true;
};

tc::vector<struct TelegramDecodedObject> telegramObjects;

unsigned int currentCRC = 0;



