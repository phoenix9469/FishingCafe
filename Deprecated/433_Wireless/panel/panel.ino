#include <Adafruit_MCP23X17.h>
Adafruit_MCP23X17 mcp;
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);
#include <SoftwareSerial.h>
SoftwareSerial rfid(2, 3);
SoftwareSerial uart(6, 5);

long time = 0;
long debounce = 50;
String rfidread;
int netbtn = 0;
int netled = 1;
int netst = LOW;
int netprv = HIGH;
int tarebtn = 8;
int tareled = 9;
int tarest = LOW;
int tareprv = HIGH;
int btn1 = 2;
int btn1led = 3;
int btn1st = LOW;
int btn1prv = HIGH;
int btn2 = 10;
int btn2led = 11;
int btn2st = LOW;
int btn2prv = HIGH;
int btn3 = 4;
int btn3led = 5;
int btn3st = LOW;
int btn3prv = HIGH;
int btn4 = 12;
int btn4led = 13;
int btn4st = LOW;
int btn4prv = HIGH;
int buzzer = 9;

void setup() {

  tone(buzzer, 2680);
  delay(100);
  noTone(buzzer);
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("INIT...");
  rfid.begin(9600);
  rfid.setTimeout(20);
  uart.begin(4800);
  Serial.begin(9600);
  Serial.println("INIT...");
  if (!mcp.begin_I2C()) {
    Serial.println("MCP INIT FAIL");
    while (1);
  }
  pinMode(LED_BUILTIN, OUTPUT);
  mcp.pinMode(netbtn, INPUT_PULLUP);
  mcp.pinMode(netled, OUTPUT);
  mcp.pinMode(tarebtn, INPUT_PULLUP);
  mcp.pinMode(tareled, OUTPUT);
  mcp.pinMode(btn1, INPUT_PULLUP);
  mcp.pinMode(btn1led, OUTPUT);
  mcp.pinMode(btn2, INPUT_PULLUP);
  mcp.pinMode(btn2led, OUTPUT);
  mcp.pinMode(btn3, INPUT_PULLUP);
  mcp.pinMode(btn3led, OUTPUT);
  mcp.pinMode(btn4, INPUT_PULLUP);
  mcp.pinMode(btn4led, OUTPUT);
  uart.listen();
  Serial.println("READY");
  lcd.clear();
}

//debug
/**void loop() {
  if (rfid.available()){
    lcd.setCursor(0,1);
    lcd.print(rfid.readStringUntil('\n'));
  }
  // LOW = pressed, HIGH = not pressed
  if (mcp.digitalRead(netbtn) == LOW) {
    lcd.setCursor(0,0);
    lcd.print("0");
    mcp.digitalWrite(netled, HIGH);
    tone(buzzer,2680);
    delay(200);
  }else{
    lcd.setCursor(0,0);
    lcd.print("1");
    mcp.digitalWrite(netled, LOW);
    noTone(buzzer);
  }

  if (mcp.digitalRead(tarebtn) == LOW) {
    lcd.setCursor(2,0);
    lcd.print("0");
    mcp.digitalWrite(tareled, HIGH);
    tone(buzzer,2680);
    delay(200);
  }else{
    lcd.setCursor(2,0);
    lcd.print("1");
    mcp.digitalWrite(tareled, LOW);
    noTone(buzzer);
  }

  if (mcp.digitalRead(btn1) == LOW) {
    lcd.setCursor(4,0);
    lcd.print("0");
    mcp.digitalWrite(btn1led, HIGH);
    tone(buzzer,2680);
    delay(200);
  }else{
    lcd.setCursor(4,0);
    lcd.print("1");
    mcp.digitalWrite(btn1led, LOW);
    noTone(buzzer);
  }

  if (mcp.digitalRead(btn2) == LOW) {
    lcd.setCursor(6,0);
    lcd.print("0");
    mcp.digitalWrite(btn2led, HIGH);
    tone(buzzer,2680);
    delay(200);
  }else{
    lcd.setCursor(6,0);
    lcd.print("1");
    mcp.digitalWrite(btn2led, LOW);
    noTone(buzzer);
  }

  if (mcp.digitalRead(btn3) == LOW) {
    lcd.setCursor(8,0);
    lcd.print("0");
    mcp.digitalWrite(btn3led, HIGH);
    tone(buzzer,2680);
    delay(200);
  }else{
    lcd.setCursor(8,0);
    lcd.print("1");
    mcp.digitalWrite(btn3led, LOW);
    noTone(buzzer);
  }

  if (mcp.digitalRead(btn4) == LOW) {
    lcd.setCursor(10,0);
    lcd.print("0");
    mcp.digitalWrite(btn4led, HIGH);
    tone(buzzer,2680);
    delay(200);
  }else{
    lcd.setCursor(10,0);
    lcd.print("1");
    mcp.digitalWrite(btn4led, LOW);
    noTone(buzzer);
  }

  }
**/

void loop() {
  btnread();
  if (uart.available()) {
    String anton = uartparse();
    if (anton == "READY") {
      digitalWrite(LED_BUILTIN, HIGH);
      lcd.setCursor(0, 0);
      lcd.print("READY!!!");
      while (1) {
        rfid.listen();
        if (rfid.available()) {
          tone(buzzer, 2680);
          delay(100);
          noTone(buzzer);
          rfidread = rfid.readString();
          btnread();
          String rfid = rfidparse();
          btnread();
          uart.listen();
          btnread();
          uart.print("RF");
          btnread();
          uart.println(rfid);
          btnread();
          rfidread = " ";
          break;
        }
        btnread();
      }
      btnread();
    }
    else if (anton == "TIMEO") {
      netst = LOW;
      btn1st = LOW;
      btn2st = LOW;
      btn3st = LOW;
      btn4st = LOW;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("TIMEOUT");
      tone(buzzer, 2680);
      delay(200);
      noTone(buzzer);
    }
    else if (anton == "ERROR") {
      netst = LOW;
      btn1st = LOW;
      btn2st = LOW;
      btn3st = LOW;
      btn4st = LOW;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("ERROR");
      tone(buzzer, 2680);
      delay(200);
      noTone(buzzer);
    }
    else if (anton == "BTNDT") {
      uart.listen();
      if (netst == LOW && btn1st == HIGH) {
        uart.print("01");
      } else if (netst == LOW && btn2st == HIGH) {
        uart.print("02");
      } else if (netst == LOW && btn3st == HIGH) {
        uart.print("03");
      } else if (netst == LOW && btn4st == HIGH) {
        uart.print("04");
      } else if (netst == HIGH && btn1st == HIGH) {
        uart.print("11");
      } else if (netst == HIGH && btn2st == HIGH) {
        uart.print("12");
      } else if (netst == HIGH && btn3st == HIGH) {
        uart.print("13");
      } else if (netst == HIGH && btn4st == HIGH) {
        uart.print("14");
      } else if (netst == HIGH && btn1st == LOW && btn2st == LOW && btn3st == LOW && btn4st == LOW) {
        uart.print("10");
      } else {
        uart.print("00");
      }
    } else {
      netst = LOW;
      btn1st = LOW;
      btn2st = LOW;
      btn3st = LOW;
      btn4st = LOW;
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(anton);
      lcd.setCursor(5, 0);
      lcd.print("KG");
    }
  }
  btnread();
}
