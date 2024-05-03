#define DEBUG
// Update treshold in milliseconds, messages will only be sent on this interval
#define UPDATE_INTERVAL 1000 // 1 second
//#define UPDATE_INTERVAL 10000 // 10 seconds
//#define UPDATE_INTERVAL 60000  // 1 minute
//#define UPDATE_INTERVAL 300000 // 5 minutes

// Update treshold in milliseconds,
// this will also send values that are more than the tresholds time the same
#define UPDATE_FULL_INTERVAL 600000 // 10 minutes
// #define UPDATE_FULL_INTERVAL 1800000 // 30 minutes
// #define UPDATE_FULL_INTERVAL 3600000 // 1 Hour

#define HOSTNAME "p1meter"
#define OTA_PASSWORD "admin"

#define BAUD_RATE 115200
#define RXD2 16
#define TXD2 17
#define P1_MAXLINELENGTH 1050

#define MQTT_MAX_RECONNECT_TRIES 100
#define MQTT_ROOT_TOPIC "sensors/power/p1meter"

#define NUMBER_OF_READOUTS 19

int LED_BUILTIN = 2;

long LAST_RECONNECT_ATTEMPT = 0;
long LAST_UPDATE_SENT = 0;
long LAST_FULL_UPDATE_SENT = 0;

char WIFI_SSID[32] = "";
char WIFI_PASS[32] = "";

char MQTT_HOST[64] = "";
char MQTT_PORT[6] = "";
char MQTT_USER[32] = "";
char MQTT_PASS[32] = "";

char telegram[P1_MAXLINELENGTH];

struct TelegramDecodedObject
{
  String name;
  long value;
  char code[16];
  char startChar = '(';
  char endChar = ')';
  bool sendData = true;
};

struct TelegramDecodedObject telegramObjects[NUMBER_OF_READOUTS];

unsigned int currentCRC = 0;
