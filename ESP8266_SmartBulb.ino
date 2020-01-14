// ESP8266 SmartBulb
//
//

// Enable diagnostic messages to serial
#define __DEBUG__

// Firmware data
const char BUILD[] = __DATE__ " " __TIME__;
#define FW_NAME         "smartbulb"
#define FW_VERSION      "0.0.1"

#define BUZZER_PIN D3

#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

// ArduinoJson
// https://arduinojson.org/
#include <ArduinoJson.h>

// NTP ClientLib 
// https://github.com/gmag11/NtpClient
#include <NtpClientLib.h>

// MQTT PubSub client
// https://github.com/knolleary/pubsubclient
#include <PubSubClient.h>

WiFiClient wifiClient;
PubSubClient client(wifiClient);

// TaskScheduler
// https://github.com/arkhipenko/TaskScheduler
#include <TaskScheduler.h>

Scheduler runner;

#define ENV_INTERVAL 30000 // Every 30 seconds
void mqttCallback();
Task mqttTask(ENV_INTERVAL, TASK_FOREVER, &mqttCallback);

#define LG_INTERVAL 30000 // LightGame change interval - every 15 seconds
void lgCallback();
Task lgTask(LG_INTERVAL, TASK_FOREVER, &lgCallback);


// I2C MPU6050
#include <Wire.h>
#include <MPU6050.h>

MPU6050 mpu;

// TimeALarm
#include <TimeAlarms.h>

// NTP
NTPSyncEvent_t ntpEvent;
bool syncEventTriggered = false; // True if a time even has been triggered

// Web server
AsyncWebServer server(80);

struct _Alarm {
  bool enabled;
  uint8_t hour;
  uint8_t minute;
  uint8_t dow;
  uint8_t mode;
};

// Config
struct _Config {
  // WiFi config
  char wifi_essid[16];
  char wifi_password[16];
  //
  char hostname[32];
  // MQTT config
  char broker_host[32];
  unsigned int broker_port;
  char client_id[18];
  // NTP Config
  char ntp_server[16];
  int8_t ntp_timezone;
  // Alarm
  _Alarm alarms[5];
};

#define CONFIG_FILE "/config.json"

File configFile;
_Config config; // Global config object

#define LED_MODE_NONE 0
#define LED_MODE_GAME 1
#define LED_MODE_MOOD 2
#define LED_MODE_MANUAL 3

long last, inactivityTimer=0;
bool actAllow=true;

// Environmental values
DynamicJsonDocument env(128);

// Format SPIFFS if mount failed
#define FORMAT_SPIFFS_IF_FAILED 1

// ************************************
// DEBUG(message)
//
// ************************************
void DEBUG(String message) {
#ifdef __DEBUG__
  Serial.println(message);    
#endif
}

// ************************************
// connectToWifi()
//
// connect to configured WiFi network
// ************************************
bool connectToWifi() {
  uint8_t timeout=0;

  if(strlen(config.wifi_essid) > 0) {
    DEBUG("[INIT] Connecting to "+String(config.wifi_essid));
    
    WiFi.mode(WIFI_STA);
    WiFi.begin(config.wifi_essid, config.wifi_password);

    while((WiFi.status() != WL_CONNECTED)&&(timeout < 10)) {
      delay(500);
      timeout++;
    }
    if(WiFi.status() == WL_CONNECTED) {
      DEBUG("OK. IP:"+WiFi.localIP().toString());

      if (MDNS.begin(config.hostname)) {
        DEBUG("[INIT] MDNS responder started");
        // Add service to MDNS-SD
        MDNS.addService("http", "tcp", 80);
      }

      // NTP    
      NTP.onNTPSyncEvent ([](NTPSyncEvent_t event) {
        ntpEvent = event;
        syncEventTriggered = true;
      });

      // NTP
      Serial.println("[INIT] Start sync NTP time...");
      NTP.begin (config.ntp_server, config.ntp_timezone, true, 0);
      NTP.setInterval(63);
      
      return true;  
    } else {
      DEBUG("[ERROR] Failed to connect to WiFi");
      return false;
    }
  } else {
    DEBUG("[ERROR] Please configure Wifi");
    return false; 
  }
}


// ************************************
// SETUP
//
// ************************************
void setup() {
  Serial.begin(115200);
  delay(10);
  
  // Buzzer PIN as output
  pinMode(BUZZER_PIN,OUTPUT);
  
  // print firmware and build data
  Serial.println();
  Serial.println();
  Serial.print(FW_NAME);  
  Serial.print(" ");
  Serial.print(FW_VERSION);
  Serial.print(" ");  
  Serial.println(BUILD);

  // Initialize SPIFFS
  if(!SPIFFS.begin()){
    DEBUG("[ERROR] SPIFFS Mount Failed. Try formatting...");
    if(SPIFFS.format()) {
      DEBUG("[INIT] SPIFFS initialized successfully");
    } else {
      DEBUG("[FATAL] SPIFFS error");
      ESP.restart();
    }
  } else {
    DEBUG("SPIFFS done");
  }

  // Load config file from SPIFFS
  loadConfigFile();

    // Setup I2C...
  DEBUG("[INIT] Initializing I2C bus...");
  Wire.begin(SDA, SCL);
  
  i2c_status();
  i2c_scanner();

  // Connect to WiFi network
  connectToWifi();
  
  // Setup OTA
  ArduinoOTA.onStart([]() { 
    Serial.println("[OTA] Update Start");
  });
  ArduinoOTA.onEnd([]() { 
    Serial.println("[OTA] Update End"); 
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    char p[32];
    sprintf(p, "[OTA] Progress: %u%%\n", (progress/(total/100)));
    Serial.println(p);
  });
  ArduinoOTA.onError([](ota_error_t error) {
    if(error == OTA_AUTH_ERROR) Serial.println("[OTA] Auth Failed");
    else if(error == OTA_BEGIN_ERROR) Serial.println("[OTA] Begin Failed");
    else if(error == OTA_CONNECT_ERROR) Serial.println("[OTA] Connect Failed");
    else if(error == OTA_RECEIVE_ERROR) Serial.println("[OTA] Recieve Failed");
    else if(error == OTA_END_ERROR) Serial.println("[OTA] End Failed");
  });
  ArduinoOTA.setHostname(config.hostname);
  ArduinoOTA.begin();
  
  // Initialize web server on port 80
  initWebServer();

  // Add environmental sensor data fetch task
  runner.addTask(mqttTask);
  mqttTask.enable();

  runner.addTask(lgTask);
  lgTask.enable();

  // Setup MQTT connection
  client.setServer(config.broker_host, config.broker_port);
  client.setCallback(mqttReceiver); 

  // Initializing MPU-6050
  if(!mpu.begin(MPU6050_SCALE_2000DPS, MPU6050_RANGE_16G)) {
    DEBUG("[ERROR] Faled initializing MPU-6050!");
  } else {
    DEBUG("[INIT] MPU-6050 initialized...");
  }

  mpu.setAccelPowerOnDelay(MPU6050_DELAY_3MS);

  mpu.setIntFreeFallEnabled(false);  
  mpu.setIntZeroMotionEnabled(false);
  mpu.setIntMotionEnabled(false);
  
  mpu.setDHPFMode(MPU6050_DHPF_5HZ);

  mpu.setMotionDetectionThreshold(10);
  mpu.setMotionDetectionDuration(5);

  mpu.setZeroMotionDetectionThreshold(10);
  mpu.setZeroMotionDetectionDuration(5);  
  mpu.setDHPFMode(MPU6050_DHPF_5HZ);

  // Define default values
  env["brightness"] = 96;
  env["speed"] = 120;
  env["mode"] = LED_MODE_NONE;
  
  // Initialize LEDs strip
  initLeds();

  // Initialize alarms

  // Go!
}

void loop() {
  // handle OTA
  ArduinoOTA.handle();
  
  // Scheduler
  runner.execute();

  // NTP ?
  if(syncEventTriggered) {
    processSyncEvent(ntpEvent);
    syncEventTriggered = false;
  }
  
  // handle MPU6050 data
  Activites act = mpu.readActivites();
  if((actAllow)&&(act.isActivity)) {
    env["mode"] = int(env["mode"])+1;
    if(env["mode"] > LED_MODE_MANUAL) { env["mode"] = LED_MODE_NONE; }
    actAllow=false;
    inactivityTimer=0;
  }
  //   
  ledLoop();
  //
  if((millis() - last) > 2100) { 
    actAllow=true;
    last = millis();
    inactivityTimer++;
  }
}
