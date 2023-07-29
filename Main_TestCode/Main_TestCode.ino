#include <Adafruit_MCP23X17.h>
#include <SoftwareSerial.h>
#include "HX711.h"
#include <Wire.h>
#include <EEPROM.h>
// EEPROM 0:Callibrate Value

// GPIO
#define HX711_DAT 4
#define HX711_CLK 5
#define PC_RX 8
#define PC_TX 7
#define RST_HWPIN 9
// MCP23017 Pins
#define SET 0
#define BUZZER 1
#define MOTOR_CONTROL 8
#define BUTTON_2 9    // WIFI,433 SW
#define BUTTON_3 10   // WIFI,433 SW
#define BUTTON_4 11   // WIFI,433 SW
#define BUTTON_5 12   // WIFI,433 SW
#define SCALE_MODE 13 // WIFI SW
#define USB_POWER 14
#define TARE 15

unsigned char uart_buffer[30] = {0x00};
bool UARTeventFlag = 0;
float CAL_VALUE = 0.0;

Adafruit_MCP23X17 mcp;
HX711 scale;
SoftwareSerial PC(PC_RX, PC_TX);

void setup()
{
  digitalWrite(RST_HWPIN, HIGH);
  delay(50);
  pinMode(RST_HWPIN, OUTPUT);
  mcp.begin_I2C();
  mcp.pinMode(SET, OUTPUT);
  mcp.pinMode(BUZZER, OUTPUT);
  mcp.pinMode(BUTTON_2, INPUT_PULLUP);
  mcp.pinMode(BUTTON_3, INPUT_PULLUP);
  mcp.pinMode(BUTTON_4, INPUT_PULLUP);
  mcp.pinMode(BUTTON_5, INPUT_PULLUP);
  mcp.pinMode(USB_POWER, OUTPUT);
  mcp.pinMode(SCALE_MODE, INPUT_PULLUP);
  mcp.pinMode(MOTOR_CONTROL, OUTPUT);
  mcp.pinMode(TARE, INPUT_PULLUP);
  mcp.digitalWrite(SET, HIGH);
  Serial.begin(4800); // Panel
  PC.begin(9600);     // HC-12
  PC.println("Test Code Start");
  EEPROM.get(0, CAL_VALUE);
  PC.print("EEPROM addr 0 Value:");
  PC.println(CAL_VALUE);
  scale.begin(HX711_DAT, HX711_CLK);
  scale.set_scale(CAL_VALUE);
  scale.tare();
}

void loop()
{
    PC.println("___________________________________________________________");
    PC.print("Callibration Value:");
    PC.println(CAL_VALUE);
    PC.print("HX711 Raw Value:");
    PC.println(scale.read());
    PC.print("HX711 Reading:");
    PC.println(scale.get_units(5), 3);
    PC.print("MCP23017 Input Reading:");
    PC.println("BTN2|BTN3|BTN4|BTN5|MODE|TARE");
    char mcp_val[12]={0x00,0x7C,0x00,0x7C,0x00,0x7C,0x00,0x7C,0x00,0x7C,0x00,0x7C};
    mcp_val[0] = mcp.digitalRead(BUTTON_2);
    mcp_val[2] = mcp.digitalRead(BUTTON_3);
    mcp_val[4] = mcp.digitalRead(BUTTON_4);
    mcp_val[6] = mcp.digitalRead(BUTTON_5);
    mcp_val[8] = mcp.digitalRead(SCALE_MODE);
    mcp_val[10] = mcp.digitalRead(TARE);
    PC.write(mcp_val, sizeof(mcp_val));
    PC.print("\n");
    PC.println("___________________________________________________________");

    if(PC.available())
    {
        PC.readBytesUntil(0x17, uart_buffer, 35);
        UARTeventFlag = 1;
    }

    if (UARTeventFlag == 1)
    {
        if(uart_buffer[1] == 0x11)//Tare
        {
            PC.println("Tare Sign RCVD");
            scale.tare();
        }
        if(uart_buffer[1] == 0x12)//GPIO Control
        {
            PC.println("Output Control Sign RCVD");
            if(uart_buffer[2] == 0x30)
            {
                mcp.digitalWrite(SET,uart_buffer[3]);
            }
            if(uart_buffer[2] == 0x31)
            {
                mcp.digitalWrite(BUZZER,uart_buffer[3]);
            }
            if(uart_buffer[2] == 0x32)
            {
                mcp.digitalWrite(USB_POWER,uart_buffer[3]);
            }
            if(uart_buffer[2] == 0x33)
            {
                mcp.digitalWrite(MOTOR_CONTROL,uart_buffer[3]);
            }
        }
        if(uart_buffer[1] == 0x13)//EEPROM Set
        {
            PC.println("EEPROM Sign RCVD");
            char eeprom_value[8] = {0x00};
            float eeprom_value_float = 0.0;
            for(int i=0;i<8;i++)
            {
                eeprom_value[i] = uart_buffer[i+3];
            }
            eeprom_value_float = atof(eeprom_value);
            PC.println(eeprom_value_float);
            EEPROM.put(uart_buffer[2],eeprom_value_float);
        }
        uart_buffer[30] = {0x00};
        UARTeventFlag = 0;
    }
}