#include <Wire.h>
#include <Adafruit_MCP23X17.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// GPIO
#define RFID_RX 2
#define RFID_TX 3
#define UART_RX 6
#define UART_TX 5
#define BUZZER 9

// MCP23017 Pins
#define NET_BTN 0
#define NET_LED 1
#define TARE_BTN 8
#define TARE_LED 9
#define USER1_BTN 2
#define USER1_LED 3
#define USER2_BTN 10
#define USER2_LED 11
#define USER3_BTN 4
#define USER3_LED 5
#define USER4_BTN 12
#define USER4_LED 13

// Test Pin
#define NET_BTN_TEST 10
#define NET_LED_TEST 4
#define TARE_BTN_TEST 11
#define TARE_LED_TEST 7
#define USER1_BTN_TEST 12
#define USER1_LED_TEST 8

Adafruit_MCP23X17 mcp;
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial rfid(RFID_RX, RFID_TX);
SoftwareSerial uart(UART_RX, UART_TX);

bool UARTeventFlag = 0;
bool UARTstatus = 0; // 0=uart 1=rfid
bool RFIDeventFlag = 0;
bool NET_BTN_STATUS = 0;
bool TARE_BTN_STATUS = 0;
bool USER1_BTN_STATUS = 0;
bool USER2_BTN_STATUS = 0;
bool USER3_BTN_STATUS = 0;
bool USER4_BTN_STATUS = 0;
bool BTN_LOCK = 0;
unsigned char uart_buffer[35] = {0x00};
unsigned char rfid_buffer[11] = {0x00};
unsigned char TARE_BTN_VALUE = {0x00};
unsigned char CAL_MODE_CNT = 0;
unsigned long debounce_time = 0;
void setup()
{
  //---------------------------------------------------------------mcp.begin_I2C();
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("INIT...");
  rfid.begin(9600);
  rfid.setTimeout(20);
  uart.begin(4800);
  Serial.begin(9600);
  Serial.println("INIT...");
  pinMode(LED_BUILTIN, OUTPUT);
  //---------------------------------------------------------------
  /*
  mcp.pinMode(NET_BTN, INPUT_PULLUP);
  mcp.pinMode(NET_LED, OUTPUT);
  mcp.pinMode(TARE_BTN, INPUT_PULLUP);
  mcp.pinMode(TARE_LED, OUTPUT);
  mcp.pinMode(USER1_BTN, INPUT_PULLUP);
  mcp.pinMode(USER1_LED, OUTPUT);
  mcp.pinMode(USER2_BTN, INPUT_PULLUP);
  mcp.pinMode(USER2_LED, OUTPUT);
  mcp.pinMode(USER3_BTN, INPUT_PULLUP);
  mcp.pinMode(USER3_LED, OUTPUT);
  mcp.pinMode(USER4_BTN, INPUT_PULLUP);
  mcp.pinMode(USER4_LED, OUTPUT);
  */
  // Test
  pinMode(NET_BTN_TEST, INPUT_PULLUP);
  pinMode(TARE_BTN_TEST, INPUT_PULLUP);
  pinMode(USER1_BTN_TEST, INPUT_PULLUP);
  pinMode(NET_LED_TEST, OUTPUT);
  pinMode(TARE_LED_TEST, OUTPUT);
  pinMode(USER1_LED_TEST, OUTPUT);
  ledControl(1);
  Serial.println("READY");
  lcd.clear();
  uart.listen();
  UARTstatus = 0;
  BTN_LOCK = 1;
}

void buttonRead()
{

  if (digitalRead(NET_BTN_TEST) == LOW && millis() - debounce_time > 50)
  {
    CAL_MODE_CNT++;
    if (NET_BTN_STATUS == 1)
    {
      NET_BTN_STATUS = 0;
    }
    else if (NET_BTN_STATUS == 0)
    {
      NET_BTN_STATUS = 1;
    }
    debounce_time = millis();
    tone(BUZZER, 2680);
    delay(200);
  }

  else if (digitalRead(TARE_BTN_TEST) == LOW && millis() - debounce_time > 50)
  {
    tone(BUZZER, 2680);
    digitalWrite(TARE_LED_TEST, HIGH);
    uart.listen();
    unsigned char tare_message[4] = {0x02, 0x16, 0x03, 0x17};
    uart.write(tare_message, sizeof(tare_message));
    if (UARTstatus == 1)
    {
      rfid.listen();
    }

    delay(200);
    digitalWrite(TARE_LED_TEST, LOW);
  }

  else if (digitalRead(USER1_BTN_TEST) == LOW && millis() - debounce_time > 50)
  {
    if (CAL_MODE_CNT >= 10)
    {
      BTN_LOCK = 1;
      CAL_MODE_CNT = 0;
      uart.listen();
      unsigned char cal_message[4] = {0x02, 0x14, 0x03, 0x17};
      uart.write(cal_message, sizeof(cal_message));
    }
    if (USER1_BTN_STATUS == 1)
    {
      USER1_BTN_STATUS = 0;
    }
    else if (USER1_BTN_STATUS == 0)
    {
      USER1_BTN_STATUS = 1;
      USER2_BTN_STATUS = 0;
      USER3_BTN_STATUS = 0;
      USER4_BTN_STATUS = 0;
    }
    debounce_time = millis();
    tone(BUZZER, 2680);
    delay(200);
  }
    noTone(BUZZER);
  //---------------------------------------------------------------
  /*
  if (mcp.digitalRead(NET_BTN) == LOW && millis() - debounce_time > 50)
  {
    if (NET_BTN_STATUS == 1)
    {
      NET_BTN_STATUS = 0;
    }
    else if (NET_BTN_STATUS == 0)
    {
      NET_BTN_STATUS = 1;
    }
    debounce_time = millis();
    tone(BUZZER, 2680);
    delay(200);
  }
  else
  {
    noTone(BUZZER);
  }

  if (mcp.digitalRead(TARE_BTN) == LOW && millis() - debounce_time > 50)
  {
    tone(BUZZER, 2680);
    mcp.digitalWrite(TARE_LED, HIGH);
    uart.listen();
    unsigned char tare_message[4] = {0x02, 0x16, 0x03, 0x17};
    uart.write(tare_message, sizeof(tare_message));
    if (UARTstatus == 1)
    {
      rfid.listen();
    }

    delay(200);
    mcp.digitalWrite(TARE_LED, LOW);
  }
  else
  {
    noTone(BUZZER);
  }

  if (mcp.digitalRead(USER1_BTN) == LOW && millis() - debounce_time > 50)
  {
    if (USER1_BTN_STATUS == 1)
    {
      USER1_BTN_STATUS = 0;
    }
    else if (USER1_BTN_STATUS == 0)
    {
      USER1_BTN_STATUS = 1;
      USER2_BTN_STATUS = 0;
      USER3_BTN_STATUS = 0;
      USER4_BTN_STATUS = 0;
    }
    debounce_time = millis();
    tone(BUZZER, 2680);
    delay(200);
  }
  else
  {
    noTone(BUZZER);
  }

  if (mcp.digitalRead(USER2_BTN) == LOW && millis() - debounce_time > 50)
  {
    if (USER2_BTN_STATUS == 1)
    {
      USER2_BTN_STATUS = 0;
    }
    else if (USER2_BTN_STATUS == 0)
    {
      USER1_BTN_STATUS = 0;
      USER2_BTN_STATUS = 1;
      USER3_BTN_STATUS = 0;
      USER4_BTN_STATUS = 0;
    }
    debounce_time = millis();
    tone(BUZZER, 2680);
    delay(200);
  }
  else
  {
    noTone(BUZZER);
  }

  if (mcp.digitalRead(USER3_BTN) == LOW && millis() - debounce_time > 50)
  {
    if (USER3_BTN_STATUS == 1)
    {
      USER3_BTN_STATUS = 0;
    }
    else if (USER3_BTN_STATUS == 0)
    {
      USER1_BTN_STATUS = 0;
      USER2_BTN_STATUS = 0;
      USER3_BTN_STATUS = 1;
      USER4_BTN_STATUS = 0;
    }
    debounce_time = millis();
    tone(BUZZER, 2680);
    delay(200);
  }
  else
  {
    noTone(BUZZER);
  }

  if (mcp.digitalRead(USER4_BTN) == LOW && millis() - debounce_time > 50)
  {
    if (USER4_BTN_STATUS == 1)
    {
      USER4_BTN_STATUS = 0;
    }
    else if (USER4_BTN_STATUS == 0)
    {
      USER1_BTN_STATUS = 0;
      USER2_BTN_STATUS = 0;
      USER3_BTN_STATUS = 0;
      USER4_BTN_STATUS = 1;
    }
    debounce_time = millis();
    tone(BUZZER, 2680);
    delay(200);
  }
  else
  {
    noTone(BUZZER);
  }
  */
}

void ledControl(unsigned char led_status)
{
  if (led_status == 0 || led_status == 1)
  {
    digitalWrite(NET_LED_TEST, led_status);
    digitalWrite(TARE_LED_TEST, led_status);
    digitalWrite(USER1_LED_TEST, led_status);
  }
  else if (led_status == 2)
  {
    digitalWrite(NET_LED_TEST, NET_BTN_STATUS);
    digitalWrite(USER1_LED_TEST, USER1_BTN_STATUS);
  }
  //---------------------------------------------------------------
  /*
  if (led_status == 0 || led_status == 1)
  {
    mcp.digitalWrite(NET_LED, LOW);
    mcp.digitalWrite(TARE_LED, LOW);
    mcp.digitalWrite(USER1_LED, LOW);
    mcp.digitalWrite(USER2_LED, LOW);
    mcp.digitalWrite(USER3_LED, LOW);
    mcp.digitalWrite(USER4_LED, LOW);
  }
  else if (led_status == 2)
  {
    mcp.digitalWrite(NET_LED, NET_BTN_STATUS);
    mcp.digitalWrite(USER1_LED, USER1_BTN_STATUS);
    mcp.digitalWrite(USER2_LED, USER2_BTN_STATUS);
    mcp.digitalWrite(USER3_LED, USER3_BTN_STATUS);
    mcp.digitalWrite(USER4_LED, USER4_BTN_STATUS);
  }
  */
}

void loop()
{
  if (BTN_LOCK == 0)
  {
    buttonRead();
    ledControl(2);
  }

  if (uart.available())
  {
    digitalWrite(LED_BUILTIN, HIGH);
    uart.readBytesUntil(0x17, uart_buffer, 35);
    // Serial.write(uart_buffer, sizeof(uart_buffer));
    UARTeventFlag = 1;
  }

  if (rfid.available())
  {
    rfid.readBytes(rfid_buffer, 10);
    RFIDeventFlag = 1;
  }

  if (RFIDeventFlag == 1) // RFID 데이터 들어오면
  {
    tone(BUZZER, 2680);
    delay(100);
    noTone(BUZZER);
    uart.listen();
    UARTstatus = 0;
    unsigned char rfid_data[14] = {0x02, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x17};
    for (int i = 0; i < 10; i++)
    {
      rfid_data[i + 2] = rfid_buffer[i];
    }
    lcd.clear();
    for (int i = 0; i < 10; i++)
    {
      lcd.write(rfid_buffer[i]);
    }
    lcd.setCursor(0, 1);
    lcd.print("TAGGED");
    uart.write(rfid_data, sizeof(rfid_data));
    RFIDeventFlag = 0;
  }

  if (UARTeventFlag == 1)
  {
    if (uart_buffer[1] == 0x05) // INIT?
    {
      ledControl(0);
      tone(BUZZER, 2680);
      delay(100);
      noTone(BUZZER);
      NET_BTN_STATUS = 0;
      USER1_BTN_STATUS = 0;
      USER2_BTN_STATUS = 0;
      USER3_BTN_STATUS = 0;
      USER4_BTN_STATUS = 0;
      CAL_MODE_CNT = 0;
      BTN_LOCK = 0;
      lcd.clear();
      rfid_buffer[11] = {0x00};
      uart_buffer[35] = {0x00};
      uart.listen();
      UARTstatus = 0;
      unsigned char init_message[4] = {0x02, 0x06, 0x03, 0x17};
      uart.write(init_message, sizeof(init_message));
      lcd.setCursor(0, 0);
      lcd.print("READY!!!");
      UARTeventFlag = 0;
      rfid.listen();
      UARTstatus = 1;
    }

    if (uart_buffer[1] == 0x12) // Button Data Request
    {
      uart.listen();
      UARTstatus = 0;
      unsigned char button_message[6] = {0x02, 0x13, 0x00, 0x00, 0x03, 0x17};
      if (NET_BTN_STATUS == 1)
      {
        button_message[2] = 0xFF;
      }
      if (USER1_BTN_STATUS == 1)
      {
        button_message[3] = 0x31;
      }
      else if (USER2_BTN_STATUS == 1)
      {
        button_message[3] = 0x32;
      }
      else if (USER3_BTN_STATUS == 1)
      {
        button_message[3] = 0x33;
      }
      else if (USER4_BTN_STATUS == 1)
      {
        button_message[3] = 0x34;
      }else{
        button_message[3] = 0x30;
      }
      uart.write(button_message, sizeof(button_message));
      uart_buffer[35] = {0x00};
      UARTeventFlag = 0;
      BTN_LOCK = 1;
    }

    if (uart_buffer[1] == 0x19) // CLCD Message Display
    {
      unsigned char UART_len = 0;
      lcd.clear();
      lcd.setCursor(0, 0);
      while (1)
      {
        if (uart_buffer[UART_len + 2] == 0x03)
        {
          break;
        }
        else
        {
          if (UART_len > 15)
          {
            lcd.setCursor(UART_len - 16, 1);
            lcd.write(uart_buffer[UART_len + 2]);
          }
          lcd.setCursor(UART_len, 0);
          lcd.write(uart_buffer[UART_len + 2]);
        }
        UART_len++;
      }
      uart_buffer[35] = {0x00};
      UARTeventFlag = 0;
    }

    if (uart_buffer[1] == 0x06) // ACK
    {
      uart_buffer[35] = {0x00};
      UARTeventFlag = 0;
    }

    if (uart_buffer[1] == 0x15) // NAK
    {
      uart_buffer[35] = {0x00};
      UARTeventFlag = 0;
    }
    UARTeventFlag = 0;
  }
}

// void serialEvent() {
//   //digitalWrite(13,HIGH);
//   Serial.readBytesUntil(0x04, uart_buffer,20);
//   //Serial.write(uart_buffer,20);
//   UARTeventFlag = 1;
// }
