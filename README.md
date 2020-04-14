# ESP8266_SmartBulb
A nice IoT "smart bulb" for your home 
v0.0.3

![ESP8266_SmartBulb](https://raw.githubusercontent.com/michelep/ESP8266_SmartBulb/master/images/1.jpg)

## FEATURES

Play nice lightshows on WS2812B "NeoPixel" LEDs, using (FastLed library)[http://fastled.io]. Based on ESP8266, it include MPU6050 module for "motion" responsive mode change and a premaplified microphone module MAX9814 to sound-based lightshow

![ESP8266_SmartBulb](https://raw.githubusercontent.com/michelep/ESP8266_SmartBulb/master/images/2.jpg)

More details on [zerozone.it](https://www.zerozone.it)

## INSTALL

Just compile and flash the code using Arduino IDE with: WeMos D1 R2 & Mini, flash size 4MB with OTA (FS:2M OTA:1MB)

## HARDWARE

- WeMos D1 Mini
- MPU 5060 Gyro module (via i2c)
- MAX9814 preamp microphone module, to be connected to A0
- WS2812 NeoPixel ring

![Schema](https://raw.githubusercontent.com/michelep/ESP8266_SmartBulb/master/images/schema.png)

![ESP8266_SmartBulb](https://raw.githubusercontent.com/michelep/ESP8266_SmartBulb/master/images/3.jpg)

