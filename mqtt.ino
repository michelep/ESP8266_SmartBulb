// ************************************
// mqtt
//
//
// ************************************

bool mqttConnect() {
  uint8_t timeout=10;
  if(strlen(config.broker_host) > 0) {
    DEBUG("[MQTT] Attempting connection to "+String(config.broker_host)+":"+String(config.broker_port));
    while((!client.connected())&&(timeout > 0)) {
      // Attempt to connect
      if (client.connect(config.client_id)) {
        // Once connected, publish an announcement...
        DEBUG("[MQTT] Connected as "+String(config.client_id));
      } else {
        timeout--;
        delay(500);
      }
    }
    if(!client.connected()) {
      DEBUG("[MQTT] Connection failed");    
      return false;
    }
    return true;
  }
}

void mqttReceiver(char* topic, byte* payload, unsigned int length) {
  DEBUG("Message arrived: "+String(topic));
}

bool mqttPublish(char *topic, char *payload) {
  if(mqttConnect()) {
    DEBUG("[MQTT] Publish "+String(topic)+":"+String(payload));
    if(client.publish(topic,payload)) {
      DEBUG("[MQTT] Published");
      return true;
    } else {
      DEBUG("[MQTT] Publish FAILED");
      return false;
    }
  }
}

void mqttCallback() {
  char topic[32];
  DEBUG("[DEBUG] MQTT callback");
  // First check for connection
  if(WiFi.status() != WL_CONNECTED) {
    connectToWifi();
  }
  // MQTT not connected? Connect!
  if (!client.connected()) {
    mqttConnect();
  }
  // Update environment value  
  env["temp"] = mpu.readTemperature();
  env["uptime"] = millis() / 1000;

  JsonObject root = env.as<JsonObject>();
 
  for (JsonPair keyValue : root) {
    sprintf(topic,"%s/%s",config.client_id,keyValue.key().c_str());
    String value = keyValue.value().as<String>();
    if(client.publish(topic,value.c_str())) {
      DEBUG("[MQTT] Published "+String(topic)+":"+value);
    } else {
      DEBUG("[MQTT] Publish failed!");
    }
  }
}
