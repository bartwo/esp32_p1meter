#ifdef __cplusplus
  extern "C" {
#endif
    uint8_t temprature_sens_read();
#ifdef __cplusplus
}
#endif
uint8_t temprature_sens_read();

void blinkLed(int numberOfBlinks, int msBetweenBlinks) {
  int msDurationForLow=200;
  for (int i = 0; i < numberOfBlinks; i++) {
    if (i > 0) { 
      delay(msDurationForLow);
    }
    digitalWrite(LED_PIN, HIGH);
    delay(msBetweenBlinks);
    digitalWrite(LED_PIN, LOW);
  }
}

void makeSureWiFiConnected(bool setupMode) {
  if (setupMode) {
    //turn WiFi power saving mode off
    esp_wifi_set_ps(WIFI_PS_NONE);
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    delay(3000);
  }
  
  if (WiFi.status() != WL_CONNECTED) {
    blinkLed(20, 50);  // Blink fast to indicate failed WiFi connection
    WiFi.begin(WIFI_SSID, WIFI_PASS);

    int _retries = WIFI_MAX_RECONNECT_TRIES;
    while (WiFi.waitForConnectResult() != WL_CONNECTED && _retries > 0) {
#ifdef DEBUG
      Serial.print("WiFi Connection Failed! Retries left: ");
      Serial.println(_retries);
#endif
      _retries--;
      delay(3000);
    }
    if (WiFi.status() != WL_CONNECTED) {
      blinkLed(10, 50);  // Blink fast to indicate failed WiFi connection
#ifdef DEBUG
      Serial.println("Connection Failed! Rebooting...");
#endif
      ESP.restart();
    }
  }
  if (setupMode) {
    debug ("WiFi Ready. IP: " + WiFi.localIP().toString() + ", RSSI: " + String(WiFi.RSSI()));
  }
}

void sendEmailMessage(String subject, String message) {
#ifdef EMAIL_DEBUGGING
  EMailSender::EMailMessage _eMailMessage;
  _eMailMessage.subject = subject;
  _eMailMessage.message = message;

  EMailSender::Response _resp = emailSend.send(EMAIL_ADDRESS, _eMailMessage);
  if (!_resp.status) {
    Serial.println("Sending status: ");
    Serial.println(_resp.status);
    Serial.println(_resp.code);
    Serial.println(_resp.desc);
  }
#endif
  ;
}

void debug(String msg) {
#ifdef DEBUG
  unsigned int _temp = (temprature_sens_read() - 32) / 1.8;
  msg += " (T: " + String(_temp) + "C)";
  Serial.println(msg);

  #ifdef MQTT_DEBUGGING;
    sendMQTTMessage(MQTT_DEBUG_TOPIC, msg.c_str());
  #endif

  #ifdef EMAIL_DEBUGGING
    emailMessageDump += msg + "\r\n";

    if (emailMessageDump.length() > 5000) {
      sendEmailMessage(String(HOSTNAME) + " message", emailMessageDump);
      emailMessageDump = "";
    }
  #endif
#endif
  ;
}