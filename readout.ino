/**
  setupDataReadout()

  This method reads JSON string specified in DsmrMap variable in dsmr_map.h file 
    and converts to telegramDecodedObjects MQTT vector.
*/
void setupDataReadout() {
  JsonDocument _dsmrMapDocument;
  DeserializationError _error = deserializeJson(_dsmrMapDocument, DSMR_MAP);
  if (_error) {
    debug("deserializeJson() failed: ");
    debug(_error.c_str());
    return;
  }

  // Create TelegramObject vector which will be used to send MQTT messages
  const uint _count = _dsmrMapDocument.as<JsonArray>().size();
  for (int i = 0; i < _count; i++) {
    TelegramDecodedObject _tdo;
    strcpy(_tdo.code, _dsmrMapDocument.as<JsonArray>()[i]["OBIS"]);
    _tdo.name = _dsmrMapDocument.as<JsonArray>()[i]["Name"].as<String>();

    telegramObjects.push_back(_tdo);
  }

#ifdef DEBUG
  debug("MQTT Topics initialized (" + String(_count) + "):");
  for (int i = 0; i < _count; i++) {
    debug(String(MQTT_ROOT_TOPIC) + "/" + telegramObjects[i].name);
  }
#endif
}