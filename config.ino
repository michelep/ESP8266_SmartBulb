// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

#include <ArduinoJson.h>

// ************************************
// Config, save and load functions
//
// save and load configuration from config file in SPIFFS. JSON format (need ArduinoJson library)
// ************************************
bool loadConfigFile() {
  DynamicJsonDocument root(512);
  
  DEBUG("[DEBUG] loadConfigFile()");

  configFile = SPIFFS.open(CONFIG_FILE, "r");
  if (!configFile) {
    DEBUG("[CONFIG] Config file not available");
    return false;
  } else {
    // Get the root object in the document
    DeserializationError err = deserializeJson(root, configFile);
    if (err) {
      DEBUG("[CONFIG] Failed to read config file:"+String(err.c_str()));
      return false;
    } else {
      strlcpy(config.wifi_essid, root["wifi_essid"], sizeof(config.wifi_essid));
      strlcpy(config.wifi_password, root["wifi_password"], sizeof(config.wifi_password));
      strlcpy(config.hostname, root["hostname"] | "smartlamp", sizeof(config.hostname));

      strlcpy(config.broker_host, root["broker_host"], sizeof(config.broker_host));
      config.broker_port = root["broker_port"] | 1883;
      strlcpy(config.client_id, root["client_id"] | "smartbulb", sizeof(config.client_id));
      
      strlcpy(config.ntp_server, root["ntp_server"] | "time.ien.it", sizeof(config.ntp_server));
      config.ntp_timezone = root["ntp_timezone"] | 1;
      DEBUG("[INIT] Configuration loaded");
    }
  }
  configFile.close();
  return true;
}

bool saveConfigFile() {
  DynamicJsonDocument root(512);
  DEBUG("[DEBUG] saveConfigFile()");

  root["wifi_essid"] = config.wifi_essid;
  root["wifi_password"] = config.wifi_password;
  root["hostname"] = config.hostname;
  root["broker_host"] = config.broker_host;
  root["broker_port"] = config.broker_port;
  root["client_id"] = config.client_id;
  root["ntp_server"] = config.ntp_server;
  root["ntp_timezone"] = config.ntp_timezone;
  
  configFile = SPIFFS.open(CONFIG_FILE, "w");
  if(!configFile) {
    DEBUG("[CONFIG] Failed to create config file !");
    return false;
  }
  serializeJson(root,configFile);
  configFile.close();
    
  return true;
}
