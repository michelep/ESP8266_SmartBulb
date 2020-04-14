// Written by Michele <o-zone@zerozone.it> Pinassi
// Released under GPLv3 - No any warranty

String templateProcessor(const String& var)
{
  //
  // System values
  //
  if(var == "hostname") {
    return String(config.hostname);
  }
  if(var == "fw_name") {
    return String(FW_NAME);
  }
  if(var=="fw_version") {
    return String(FW_VERSION);
  }
  if(var=="uptime") {
    return String(millis()/1000);
  }
  if(var=="timedate") {
    return NTP.getTimeDateString();
  }
  //
  // Ambient values
  //
  if(var=="temp") {
    return env["temp"];
  }  
  if(var=="speed") {
    return String(config.speed);
  }  
  if(var=="brightness") {
    return String(config.brightness);
  }
  if(var=="mode") {
    return String(config.mode);
  }    
  if(var=="mood") {
    return String(config.mood);
  }    
  if(var=="red") {
    return String(config.red);
  }  
  if(var=="green") {
    return String(config.green);
  }  
  if(var=="blue") {
    return String(config.blue);
  }  
  //
  // Config values
  //
  if(var=="wifi_essid") {
    return String(config.wifi_essid);
  }
  if(var=="wifi_password") {
    return String(config.wifi_password);
  }
  if(var=="wifi_rssi") {
    return String(WiFi.RSSI());
  }
  if(var=="ntp_server") {
    return String(config.ntp_server);
  }
  if(var=="ntp_timezone") {
    return String(config.ntp_timezone);
  }
  if(var=="broker_host") {
    return String(config.broker_host);
  }
  if(var=="broker_port") {
    return String(config.broker_port);
  }  
  if(var=="client_id") {
    return String(config.client_id);
  }
  return String();
}

// ************************************
// initWebServer
//
// initialize web server
// ************************************
void initWebServer() {
  server.serveStatic("/", SPIFFS, "/").setDefaultFile("index.html").setTemplateProcessor(templateProcessor);

  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
    String message;
    if(request->hasParam("wifi_essid", true)) {
        strcpy(config.wifi_essid,request->getParam("wifi_essid", true)->value().c_str());
    }
    if(request->hasParam("wifi_password", true)) {
        strcpy(config.wifi_password,request->getParam("wifi_password", true)->value().c_str());
    }
    if(request->hasParam("ntp_server", true)) {
        strcpy(config.ntp_server, request->getParam("ntp_server", true)->value().c_str());
    }
    if(request->hasParam("ntp_timezone", true)) {
        config.ntp_timezone = atoi(request->getParam("ntp_timezone", true)->value().c_str());
    }
    if(request->hasParam("broker_host", true)) {
        strcpy(config.broker_host,request->getParam("broker_host", true)->value().c_str());
    }
    if(request->hasParam("broker_port", true)) {
        config.broker_port = atoi(request->getParam("broker_port", true)->value().c_str());
    }
    if(request->hasParam("client_id", true)) {
        strcpy(config.client_id, request->getParam("client_id", true)->value().c_str());
    }    
    //
    saveConfigFile();
    request->redirect("/?result=ok");
  });

  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
    ESP.restart();
  });
  
  server.on("/ajax", HTTP_POST, [] (AsyncWebServerRequest *request) {
    String action,value,response="";
    char outputJson[256];

    if (request->hasParam("action", true)) {
      action = request->getParam("action", true)->value();
      DEBUG("ACTION: "+String(action));
      // GET
      if(action.equals("getEnv")) {
        serializeJson(env,outputJson);
        response = String(outputJson);
      }
      // SET
      if(action.equals("setBright")) {
        config.brightness = atoi(request->getParam("value", true)->value().c_str());
        response = String("OK");
      }
      if(action.equals("setSpeed")) {
        config.speed = atoi(request->getParam("value", true)->value().c_str());        
        response = String("OK");
      }
      if(action.equals("setTimeout")) {
        config.timeout = atoi(request->getParam("value", true)->value().c_str());        
        response = String("OK");
      }
      if(action.equals("setMood")) {
        config.mood = atoi(request->getParam("value", true)->value().c_str());
        response = String("OK");
      }
      if(action.equals("setMode")) {
        config.mode = atoi(request->getParam("value", true)->value().c_str());
        response = String("OK");
      }
      if(action.equals("setR")) {
        config.red = atoi(request->getParam("value", true)->value().c_str());
        response = String("OK");
      }
      if(action.equals("setG")) {
        config.green = atoi(request->getParam("value", true)->value().c_str());        
        response = String("OK");
      }
      if(action.equals("setB")) {
        config.blue = atoi(request->getParam("value", true)->value().c_str());
        response = String("OK");
      }
      inactivityTimer=0;
    }
    request->send(200, "text/plain", response);
  });

  server.onNotFound([](AsyncWebServerRequest *request) {
    Serial.printf("NOT_FOUND: ");
    Serial.printf(" http://%s%s\n", request->host().c_str(), request->url().c_str());
    request->send(404);
  });

  server.begin();
}
