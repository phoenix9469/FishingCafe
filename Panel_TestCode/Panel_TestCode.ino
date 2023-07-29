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

Adafruit_MCP23X17 mcp;
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial rfid(RFID_RX, RFID_TX);
SoftwareSerial uart(UART_RX, UART_TX);

bool UARTeventFlag = 0;
bool RFIDeventFlag = 0;
unsigned char uart_buffer[35] = {0x00};

void setup()
{
    mcp.begin_I2C();
    lcd.init();
    lcd.backlight();
    lcd.setCursor(0, 0);
    lcd.print("INIT...");
    rfid.begin(9600);
    // rfid.setTimeout(20);필요한지 확인요망
    Serial.begin(9600);
    Serial.println("INIT...");
    pinMode(LED_BUILTIN, OUTPUT);
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
    ledControl(1);
    Serial.println("READY");
    lcd.clear();
    rfid.listen();
}

void buttonRead()
{
    if (mcp.digitalRead(NET_BTN) == LOW && millis() - debounce_time > 50)
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
}

void ledControl(unsigned char led_status)
{
    if (led_status == 0 || led_status == 1)
    {
        mcp.digitalWrite(NET_LED, led_status);
        mcp.digitalWrite(TARE_LED, led_status);
        mcp.digitalWrite(USER1_LED, led_status);
        mcp.digitalWrite(USER2_LED, led_status);
        mcp.digitalWrite(USER3_LED, led_status);
        mcp.digitalWrite(USER4_LED, led_status);
    }
    else if (led_status == 2)
    {
        mcp.digitalWrite(NET_LED, NET_BTN_STATUS);
        mcp.digitalWrite(USER1_LED, USER1_BTN_STATUS);
        mcp.digitalWrite(USER2_LED, USER2_BTN_STATUS);
        mcp.digitalWrite(USER3_LED, USER3_BTN_STATUS);
        mcp.digitalWrite(USER4_LED, USER4_BTN_STATUS);
    }
}

void serialEvent()
{
    Serial.readBytesUntil(0x17, uart_buffer, 30);
    UARTeventFlag = 1;
}

void loop()
{
    if (rfid.available())
    {
        rfid.readBytes(rfid_buffer, 10);
        RFIDeventFlag = 1;
    }
    
    if (UARTeventFlag == 1)
    {
        UARTeventFlag = 0;
    }

    if (RFIDeventFlag == 1)
    {
        RFIDeventFlag = 0;
    }
}