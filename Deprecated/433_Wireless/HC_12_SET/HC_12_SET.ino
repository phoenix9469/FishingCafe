#include <Adafruit_MCP23X17.h>
#include <SoftwareSerial.h>
Adafruit_MCP23X17 mcp;
SoftwareSerial mySerial(8,7);
void setup() {
Serial.begin(9600);
mySerial.begin(9600);
if (!mcp.begin_I2C()) {
    Serial.println("Error.");
    while (1);
  }
mcp.pinMode(0, OUTPUT);
}

void loop() {
mcp.digitalWrite(0, LOW);
if (mySerial.available()) {       
    Serial.write(mySerial.read());  //블루투스측 내용을 시리얼모니터에 출력
  }
  if (Serial.available()) {         
    mySerial.write(Serial.read());  //시리얼 모니터 내용을 블루추스 측에 WRITE
  }
  }
