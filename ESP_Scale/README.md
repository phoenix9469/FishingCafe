# FishingCafe Scale Project - ESP Scale

- Required Library(For ESP Scale)
    1. NAU7802 : https://github.com/adafruit/Adafruit_NAU7802
    2. MCP23X17 : https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library  
    3. Adafruit BusIO : https://github.com/adafruit/Adafruit_BusIO  
    4. I2C LCD : https://github.com/johnrickman/LiquidCrystal_I2C
    5. PCF8563 : https://github.com/Bill2462/PCF8563-Arduino-Library
    6. ArduinoJSON : https://github.com/bblanchon/ArduinoJson
    7. ESPAsyncWebServer : https://github.com/me-no-dev/ESPAsyncWebServer
    8. LcdMenu : https://github.com/forntoh/LcdMenu
    9. ESP32Ping : https://github.com/marian-craciunescu/ESP32Ping

### Main Device -> PC Protocol
|Data|Divider|Weight|Divider|Status(0=Normal,1=Error)|Divider|Button(0~4)|
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
|READY|\||0|\||0|\||0|

#### Normal Flow
|No|Main|Direction|PC|Description|
|:---:|:---:|:---:|:---:|:---:|
|1|READY\|0\|0\|0|->|---|Ready|
|2|5300205877\|0\|0\|0|->|---|RFID Data|
|3|5300205877\|0.123\|0\|0|->|---|Weight Measure Finish|

#### Error(Timeout, Weight Range Out)
|No|Main|Direction|PC|Description|
|:---:|:---:|:---:|:---:|:---:|
|1|READY\|0\|0\|0|->|---|Ready|
|2|5300205877\|0\|0\|0|->|---|RFID Data|
|3|5300205877\|0.000\|1\|0|->|---|Error|

### Device Image

### PCB Image
