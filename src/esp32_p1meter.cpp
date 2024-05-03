#include <Arduino.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <WiFi.h>

#include "settings.h"

WiFiClient espClient;
PubSubClient mqttClient(espClient);

void setupDataReadout();
void setupOTA();
void blinkLed(int numberOfBlinks, int msBetweenBlinks);
bool mqttReconnect();
void sendMQTTMessage();
void sendMetric(const String &name, long metric);
bool readP1Serial();
unsigned int crc16(unsigned int crc, unsigned char *buf, int len);
void sendDataToBroker();

/***********************************
            Main Setup
 ***********************************/
void setup()
{
    // Initialize pins
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
    Serial.begin(BAUD_RATE);
    Serial2.begin(BAUD_RATE, SERIAL_8N1, RXD2, TXD2, true);

#ifdef DEBUG
    Serial.println("Booting - DEBUG mode on");
    blinkLed(2, 500);
    delay(500);
    blinkLed(2, 2000);
    // Blinking 2 times fast and two times slower to indicate DEBUG mode
#endif
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
#ifdef DEBUG
        Serial.println("Connection Failed! Rebooting...");
#endif
        delay(5000);
        ESP.restart();
    }
    delay(3000);
    setupDataReadout();
    setupOTA();
    mqttClient.setServer(MQTT_HOST, atoi(MQTT_PORT));
    blinkLed(5, 500); // Blink 5 times to indicate end of setup
#ifdef DEBUG
    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
#endif
}

/***********************************
            Main Loop
 ***********************************/
void loop()
{
    long now = millis();
    if (WiFi.status() != WL_CONNECTED)
    {
        blinkLed(20, 50); // Blink fast to indicate failed WiFi connection
        WiFi.begin(WIFI_SSID, WIFI_PASS);
        while (WiFi.waitForConnectResult() != WL_CONNECTED)
        {
#ifdef DEBUG
            Serial.println("Connection Failed! Rebooting...");
#endif
            delay(5000);
            ESP.restart();
        }
    }

    ArduinoOTA.handle();

    if (!mqttClient.connected())
    {
        if (now - LAST_RECONNECT_ATTEMPT > 5000)
        {
            LAST_RECONNECT_ATTEMPT = now;

            if (!mqttReconnect())
            {
#ifdef DEBUG
                Serial.println("Connection to MQTT Failed! Rebooting...");
#endif
                delay(5000);
                ESP.restart();
            }
            else
            {
                LAST_RECONNECT_ATTEMPT = 0;
            }
        }
    }
    else
    {
        mqttClient.loop();
    }

    // Check if we want a full update of all the data including the unchanged data.
    if (now - LAST_FULL_UPDATE_SENT > UPDATE_FULL_INTERVAL)
    {
        for (int i = 0; i < NUMBER_OF_READOUTS; i++)
        {
            telegramObjects[i].sendData = true;
            LAST_FULL_UPDATE_SENT = millis();
        }
    }

    if (now - LAST_UPDATE_SENT > UPDATE_INTERVAL)
    {
        if (readP1Serial())
        {
            LAST_UPDATE_SENT = millis();
            sendDataToBroker();
        }
    }
}

/***********************************
            Setup Methods
 ***********************************/

/**
   setupDataReadout()

   This method can be used to create more data readout to mqtt topic.
   Use the name for the mqtt topic.
   The code for finding this in the telegram see
    https://www.netbeheernederland.nl/_upload/Files/Slimme_meter_15_a727fce1f1.pdf for the dutch codes pag. 19 -23
   Use startChar and endChar for setting the boundies where the value is in between.
   Default startChar and endChar is '(' and ')'
   Note: Make sure when you add or remove telegramObject to update the NUMBER_OF_READOUTS accordingly.
*/
void setupDataReadout()
{
    // 1-0:1.8.1(000992.992*kWh)
    // 1-0:1.8.1 = Elektra verbruik laag tarief (DSMR v5.0)
    telegramObjects[0].name = "consumption_tarif_1";
    strcpy(telegramObjects[0].code, "1-0:1.8.1");
    telegramObjects[0].endChar = '*';

    // 1-0:1.8.2(000560.157*kWh)
    // 1-0:1.8.2 = Elektra verbruik hoog tarief (DSMR v5.0)
    telegramObjects[1].name = "consumption_tarif_2";
    strcpy(telegramObjects[1].code, "1-0:1.8.2");
    telegramObjects[1].endChar = '*';

    telegramObjects[0].name = "received_tarif_1";
    strcpy(telegramObjects[0].code, "1-0:2.8.1");
    telegramObjects[0].endChar = '*';

    // 1-0:1.8.2(000560.157*kWh)
    // 1-0:1.8.2 = Elektra verbruik hoog tarief (DSMR v5.0)
    telegramObjects[1].name = "received_tarif_2";
    strcpy(telegramObjects[1].code, "1-0:2.8.2");
    telegramObjects[1].endChar = '*';

    // 1-0:1.7.0(00.424*kW) Actueel verbruik
    // 1-0:1.7.x = Electricity consumption actual usage (DSMR v5.0)
    telegramObjects[2].name = "actual_consumption";
    strcpy(telegramObjects[2].code, "1-0:1.7.0");
    telegramObjects[2].endChar = '*';

    // 1-0:2.7.0(00.000*kW) Actuele teruglevering (-P) in 1 Watt resolution
    telegramObjects[3].name = "actual_received";
    strcpy(telegramObjects[3].code, "1-0:2.7.0");
    telegramObjects[3].endChar = '*';

    // 1-0:21.7.0(00.378*kW)
    // 1-0:21.7.0 = Instantaan vermogen Elektriciteit levering L1
    telegramObjects[4].name = "instant_power_usage_l1";
    strcpy(telegramObjects[4].code, "1-0:21.7.0");
    telegramObjects[4].endChar = '*';

    // 1-0:41.7.0(00.378*kW)
    // 1-0:41.7.0 = Instantaan vermogen Elektriciteit levering L2
    telegramObjects[5].name = "instant_power_usage_l2";
    strcpy(telegramObjects[5].code, "1-0:41.7.0");
    telegramObjects[5].endChar = '*';

    // 1-0:61.7.0(00.378*kW)
    // 1-0:61.7.0 = Instantaan vermogen Elektriciteit levering L3
    telegramObjects[6].name = "instant_power_usage_l3";
    strcpy(telegramObjects[6].code, "1-0:61.7.0");
    telegramObjects[6].endChar = '*';

    // 1-0:22.7.0(00.378*kW)
    // 1-0:22.7.0 = Instantaan vermogen Elektriciteit teruglevering L1
    telegramObjects[4].name = "instant_power_return_l1";
    strcpy(telegramObjects[4].code, "1-0:22.7.0");
    telegramObjects[4].endChar = '*';

    // 1-0:42.7.0(00.378*kW)
    // 1-0:42.7.0 = Instantaan vermogen Elektriciteit teruglevering L2
    telegramObjects[5].name = "instant_power_return_l2";
    strcpy(telegramObjects[5].code, "1-0:42.7.0");
    telegramObjects[5].endChar = '*';

    // 1-0:62.7.0(00.378*kW)
    // 1-0:62.7.0 = Instantaan vermogen Elektriciteit teruglevering L3
    telegramObjects[6].name = "instant_power_return_l3";
    strcpy(telegramObjects[6].code, "1-0:62.7.0");
    telegramObjects[6].endChar = '*';

    // 1-0:31.7.0(002*A)
    // 1-0:31.7.0 = Instantane stroom Elektriciteit L1
    telegramObjects[7].name = "instant_power_current_l1";
    strcpy(telegramObjects[7].code, "1-0:31.7.0");
    telegramObjects[7].endChar = '*';

    // 1-0:51.7.0(002*A)
    // 1-0:51.7.0 = Instantane stroom Elektriciteit L2
    telegramObjects[8].name = "instant_power_current_l2";
    strcpy(telegramObjects[8].code, "1-0:51.7.0");
    telegramObjects[8].endChar = '*';

    // 1-0:71.7.0(002*A)
    // 1-0:71.7.0 = Instantane stroom Elektriciteit L3
    telegramObjects[9].name = "instant_power_current_l3";
    strcpy(telegramObjects[9].code, "1-0:71.7.0");
    telegramObjects[9].endChar = '*';

    // 1-0:32.7.0(232.0*V)
    // 1-0:32.7.0 = Voltage L1
    telegramObjects[10].name = "instant_voltage_l1";
    strcpy(telegramObjects[10].code, "1-0:32.7.0");
    telegramObjects[10].endChar = '*';

    // 1-0:52.7.0(232.0*V)
    // 1-0:52.7.0 = Voltage L2
    telegramObjects[11].name = "instant_voltage_l2";
    strcpy(telegramObjects[11].code, "1-0:52.7.0");
    telegramObjects[11].endChar = '*';

    // 1-0:72.7.0(232.0*V)
    // 1-0:72.7.0 = Voltage L3
    telegramObjects[12].name = "instant_voltage_l3";
    strcpy(telegramObjects[12].code, "1-0:72.7.0");
    telegramObjects[12].endChar = '*';

    // 0-0:96.14.0(0001)
    // 0-0:96.14.0 = Actual Tarif
    telegramObjects[13].name = "actual_tarif_group";
    strcpy(telegramObjects[13].code, "0-0:96.14.0");

    // 0-1:24.2.3(150531200000S)(00811.923*m3)
    // 0-1:24.2.3 = Gas (DSMR v5.0) on Belgian meters
    telegramObjects[18].name = "gas_meter_m3";
    strcpy(telegramObjects[18].code, "0-1:24.2.3");
    telegramObjects[12].endChar = '*';

#ifdef DEBUG
    Serial.println("MQTT Topics initialized:");
    for (int i = 0; i < NUMBER_OF_READOUTS; i++)
    {
        Serial.println(String(MQTT_ROOT_TOPIC) + "/" + telegramObjects[i].name);
    }
#endif
}

/**
   Over the Air update setup
*/
void setupOTA()
{
    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";

            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            Serial.println("Start updating " + type);
        })
        .onEnd([]() {
            Serial.println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR)
                Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR)
                Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR)
                Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR)
                Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR)
                Serial.println("End Failed");
        });

    ArduinoOTA.begin();
}


void sendMQTTMessage(const char *topic, const char *payload) // Fixed: payload should be const char *
    {
        bool result = mqttClient.publish(topic, payload, false);
    }

bool mqttReconnect()
{
    int MQTT_RECONNECT_RETRIES = 0;

    while (!mqttClient.connected() && MQTT_RECONNECT_RETRIES < MQTT_MAX_RECONNECT_TRIES)
    {
        MQTT_RECONNECT_RETRIES++;

        if (mqttClient.connect(HOSTNAME, MQTT_USER, MQTT_PASS))
        {
            char message[16 + strlen(HOSTNAME) + 1]; // Fixed: dynamic memory allocation is unnecessary
            strcpy(message, "p1 meter alive: ");
            strcat(message, HOSTNAME);
            mqttClient.publish("hass/status", message);
        }
        else
        {
            delay(5000);
        }
    }

    if (MQTT_RECONNECT_RETRIES >= MQTT_MAX_RECONNECT_TRIES)
    {
        return false;
    }

    return true;
}

void sendMetric(const String &name, long metric) // Fixed: name passed by reference
{
    // if (metric > 0)
    // {
    char output[10];
    ltoa(metric, output, sizeof(output));

    String topic = String(MQTT_ROOT_TOPIC) + "/" + name;
#ifdef DEBUG
    Serial.println(topic);
#endif
    sendMQTTMessage(topic.c_str(), output);
    // }
}

void sendDataToBroker()
{
    for (int i = 0; i < NUMBER_OF_READOUTS; i++)
    {
#ifdef DEBUG
        Serial.println("Sending: " + telegramObjects[i].name + " value: " + String(telegramObjects[i].value));
#endif
        if (telegramObjects[i].sendData)
        {
            sendMetric(telegramObjects[i].name, telegramObjects[i].value);
            telegramObjects[i].sendData = false;
        }
    }
}

void blinkLed(int numberOfBlinks, int msBetweenBlinks)
{
  for (int i = 0; i < numberOfBlinks; i++)
  {
    digitalWrite(LED_BUILTIN, HIGH);
    delay(msBetweenBlinks);
    digitalWrite(LED_BUILTIN, LOW);
    if (i != numberOfBlinks - 1)
    {
      delay(msBetweenBlinks);
    }
  }
}

unsigned int crc16(unsigned int crc, unsigned char *buf, int len)
{
    for (int pos = 0; pos < len; pos++)
    {
        crc ^= (unsigned int)buf[pos];

        for (int i = 8; i != 0; i--)
        {
            if ((crc & 0x0001) != 0)
            {
                crc >>= 1;
                crc ^= 0xA001;
            }
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

bool isNumber(char *res, int len)
{
    for (int i = 0; i < len; i++)
    {
        if (((res[i] < '0') || (res[i] > '9')) && (res[i] != '.' && res[i] != 0))
        {
            return false;
        }
    }
    return true;
}

int findCharInArrayRev(char array[], char c, int len)
{
    for (int i = len - 1; i >= 0; i--)
    {
        if (array[i] == c)
        {
            return i;
        }
    }

    return -1;
}

long getValue(char *buffer, int maxlen, char startchar, char endchar)
{
    int s = findCharInArrayRev(buffer, startchar, maxlen - 2);
    int l = findCharInArrayRev(buffer, endchar, maxlen - 2) - s - 1;

    char res[16];
    memset(res, 0, sizeof(res));

    if (strncpy(res, buffer + s + 1, l))
    {
        if (endchar == '*')
        {
            if (isNumber(res, l))
                return (1000 * atof(res));
        }
        else if (endchar == ')')
        {
            if (isNumber(res, l))
                return atof(res);
        }
    }

    return 0;
}

/**
 *  Decodes the telegram PER line. Not the complete message. 
 */
bool decodeTelegram(char* telegram,int len)
{
    int startChar = findCharInArrayRev(telegram, '/', len);
    int endChar = findCharInArrayRev(telegram, '!', len);
    bool validCRCFound = false;

#ifdef DEBUG
    for (int cnt = 0; cnt < len; cnt++)
    {
        Serial.print(telegram[cnt]);
    }
    Serial.print("\n");
#endif
    unsigned int currentCRC = 0;

    if (startChar >= 0)
    {
        // * Start found. Reset CRC calculation
        currentCRC = crc16(0x0000, (unsigned char *)telegram + startChar, len - startChar);
    }
    else if (endChar >= 0)
    {
        // * Add to crc calc
        currentCRC = crc16(currentCRC, (unsigned char *)telegram + endChar, 1);

        char messageCRC[5];
        strncpy(messageCRC, telegram + endChar + 1, 4);

        messageCRC[4] = 0; // * Thanks to HarmOtten (issue 5)
        validCRCFound = (strtol(messageCRC, NULL, 16) == currentCRC);

#ifdef DEBUG
        if (validCRCFound)
            Serial.println(F("CRC Valid!"));
        else
            Serial.println(F("CRC Invalid!"));
#endif
        currentCRC = 0;
    }
    else
    {
        currentCRC = crc16(currentCRC, (unsigned char *)telegram, len);
    }

    // Loops throug all the telegramObjects to find the code in the telegram line
    // If it finds the code the value will be stored in the object so it can later be send to the mqtt broker
    for (int i = 0; i < NUMBER_OF_READOUTS; i++)
    {
        if (strncmp(telegram, telegramObjects[i].code, strlen(telegramObjects[i].code)) == 0)
        {
            long newValue = getValue(telegram, len, telegramObjects[i].startChar, telegramObjects[i].endChar);
            if (newValue != telegramObjects[i].value)
            {
                telegramObjects[i].value = newValue;
                telegramObjects[i].sendData = true;
            }
            break;

#ifdef DEBUG
            Serial.println((String) "Found a Telegram object: " + telegramObjects[i].name + " value: " + telegramObjects[i].value);
#endif
        }
    }

    return validCRCFound;
}

bool readP1Serial()
{
    if (Serial2.available())
    {
#ifdef DEBUG
        Serial.println("Serial2 is available");
        Serial.println("Memset telegram");
#endif
        memset(telegram, 0, sizeof(telegram));
        while (Serial2.available())
        {
            // Reads the telegram untill it finds a return character
            // That is after each line in the telegram
            int len = Serial2.readBytesUntil('\n', telegram, P1_MAXLINELENGTH);

            telegram[len] = '\n';
            telegram[len + 1] = 0;

            bool result = decodeTelegram(telegram,len + 1);
            // When the CRC is check which is also the end of the telegram
            // if valid decode return true
            if (result)
            {
                return true;
            }
        }
    }
    return false;
}
