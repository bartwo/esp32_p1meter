unsigned int crc16(unsigned int crc, unsigned char *buf, int len) {
  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned int)buf[pos];

    for (int i = 8; i != 0; i--) {
      if ((crc & 0x0001) != 0) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  return crc;
}

short getValueType(char *res, int len) {
  short dotsFound = 0;
  for (int i = 0; i < len; i++) {
    if (((res[i] < '0') || (res[i] > '9')) && (res[i] != '.' && res[i] != 0)) {
      return -1; // non-numeric
    }
    if (res[i] == '.') {
      dotsFound++;
    }
  }
  if (dotsFound > 1) {
    return -1; // more than 1 dot means something weird, not number for sure
  }
  return dotsFound; // 1 - float/double, 0 - integer
}

int findCharInArrayRev(char array[], char c, int len) {
  for (int i = len - 1; i >= 0; i--) {
    if (array[i] == c) {
      return i;
    }
  }

  return -1;
}

String getValue(char *buffer, int maxlen, char startchar, char endchar) {
  int s = findCharInArrayRev(buffer, startchar, maxlen - 2);
  int l = findCharInArrayRev(buffer, endchar, maxlen - 2) - s - 1;

  if (l < 0) // end char not found
    return String(NA);

  char res[16];
  memset(res, 0, sizeof(res));

  if (strncpy(res, buffer + s + 1, l)) {
    short numType = getValueType(res, l);
    if (numType == 1) // float or double
      return String((endchar == VALUE_END_CHAR ? VALUE_NUMERIC_MULTIPLIER : 1) * atof(res), VALUE_FLOAT_DECIMAL_PLACES);
    else if (numType == 0) // integer
      return String((endchar == VALUE_END_CHAR ? VALUE_NUMERIC_MULTIPLIER : 1) * atoi(res));
    else
      return String(res);
  }

  return "";
}

/**
 *  Decodes the telegram PER line. Not the complete message. 
 */
bool decodeTelegram(int len) {
  int startChar = findCharInArrayRev(telegram, '/', len);
  int endChar = findCharInArrayRev(telegram, '!', len);
  bool validCRCFound = false;

#ifdef DEBUG
  for (int cnt = 0; cnt < len; cnt++) {
    Serial.print(telegram[cnt]);
  }
  Serial.println("");
#endif

  if (startChar >= 0) {
    // * Start found. Reset CRC calculation
    currentCRC = crc16(0x0000, (unsigned char *)telegram + startChar, len - startChar);
  } else if (endChar >= 0) {
    // * Add to crc calc
    currentCRC = crc16(currentCRC, (unsigned char *)telegram + endChar, 1);

    char messageCRC[5];
    strncpy(messageCRC, telegram + endChar + 1, 4);

    messageCRC[4] = 0;  // * Thanks to HarmOtten (issue 5)
    validCRCFound = (strtol(messageCRC, NULL, 16) == currentCRC);

    debug(validCRCFound ? "CRC valid!" : "CRC Invalid!");
    currentCRC = 0;
  } else {
    currentCRC = crc16(currentCRC, (unsigned char *)telegram, len);
  }

  // Loops throug all the telegramObjects to find the code in the telegram line
  // If it finds the code the value will be stored in the object so it can later be send to the mqtt broker
  for (int i = 0; i < telegramObjects.size(); i++) {
    if (strncmp(telegram, telegramObjects[i].code, strlen(telegramObjects[i].code)) == 0) {
      String newValue = getValue(telegram, len, VALUE_START_CHAR, VALUE_END_CHAR);
      if (newValue == String(NA)) {
        newValue = getValue(telegram, len, VALUE_START_CHAR, VALUE_NO_UNITS_END_CHAR);
      }
      if (newValue != telegramObjects[i].value) {
        telegramObjects[i].value = newValue;
        telegramObjects[i].sendData = true;
      }
      debug((String) "Found a Telegram object: " + telegramObjects[i].name + " value: " + telegramObjects[i].value);
      break;
    }
  }

  return validCRCFound;
}

bool readP1Serial() {
  if (Serial2.available()) {
    debug("Serial2 is available. Memset telegram.");
    memset(telegram, 0, sizeof(telegram));
    while (Serial2.available()) {
      // Reads the telegram until it finds a return character
      // That is after each line in the telegram
      int len = Serial2.readBytesUntil('\n', telegram, P1_MAXLINELENGTH);

      telegram[len] = '\n';
      telegram[len + 1] = 0;

      bool result = decodeTelegram(len + 1);
      // When the CRC is check which is also the end of the telegram
      // if valid decode return true
      if (result) {
        return true;
      }
    }
  }
  return false;
}

