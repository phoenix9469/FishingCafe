# FishingCafe Scale Project
- Required Library
    1. HX711 : https://github.com/bogde/HX711  
    2. MCP23X17 : https://github.com/adafruit/Adafruit-MCP23017-Arduino-Library  
    3. Adafruit BusIO : https://github.com/adafruit/Adafruit_BusIO  
    4. I2C LCD : https://github.com/johnrickman/LiquidCrystal_I2C
    5. SoftwareSerial.h, Wire.h

You may use Xloader to upload HEX File to board.  
https://github.com/binaryupdates/xLoader

### Main Device <-> Panel Protocol
|STX|Command|Data|ETX|ETB|
|:---:|:---:|:---:|:---:|:---:|
|0x02|Command|Data|0x03|0x17|

#### Command
|To|Command(HEX)|Description|
|:---:|:---:|:---:|
|Main|0x06|Panel Init Finished|
|Main|0x11|RFID Data|
|Main|0x13|Panel Button Data|
|Main|0x14|Callibrate Mode|
|Main|0x16|Tare Scale|
|Panel|0x05|Start Init|
|Panel|0x12|Button Data Request|
|Panel|0x19|CLCD Message Display|

#### Normal Flow
|No|Main|Direction|Panel|Description|
|:---:|:---:|:---:|:---:|:---:|
|1|0x02,0x05,0x03,0x17|->|---|Init?|
|2|---|<-|0x02,0x06,0x03,0x17|Init.|
|3|---|<-|0x02,0x11,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x17|RFID Data|
|4|0x02,0x12,0x03,0x17|->|---|Button Data?|
|5|---|<-|0x02,0x13,0x00,0x00,0x03,0x17|Button Data.([2]=0x00,0xFF NET,[3]=0x30~0x33 User BTN)|
|6|0x02,0x19,0x00,0x00,0x00,0x00,0x00,0x03,0x17|->|---|Display Weight|

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

### PCB Image
![image](https://github.com/phoenix9469/FishingCafe/assets/82319443/ba9c5953-cb6e-43f0-b427-195fef2a58e1)