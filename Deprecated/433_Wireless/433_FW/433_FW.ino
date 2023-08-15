#include <Adafruit_MCP23X17.h>
#include <SoftwareSerial.h>
#include "HX711.h"
#include <Wire.h>
//-----------------------------------------------IO PIN
#define HX711_DAT 4
#define HX711_CLK 5
#define PC_RX 8
#define PC_TX 7
#define RST_HWPIN 9
//-----------------------------------------------MCP PIN
Adafruit_MCP23X17 mcp;
#define SET 0
#define BUZZER 1
#define MOTOR_CONTROL 8
#define BUTTON_2 9
#define BUTTON_3 10
#define BUTTON_4 11
#define BUTTON_5 12
#define SCALE_MODE 13
#define USB_POWER 14
#define TARE 15
#define CAL_FACTOR -80569.0 //1:-80569.0 2:80893.0
String UART_DATA;
String UART_SIGN;
String EXT_BTN_DATA = "0";
String RFID_DATA;
int NET_FLAG = 0;
int MODE_FLAG = 0;
HX711 scale;
SoftwareSerial PC(PC_RX, PC_TX); // RX, TX
//---------------------------------------------------------
//↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓USER_PARAM
int INPUT_TIMEOUT = 10;//투입 타임아웃,초
int AVERAGE_MODE_DATA_AMOUNT = 35;//평균모드 데이터 개수,시간 계산법:35*2/10 = 7초
int STBL_MODE_DATA_AMOUNT = 15;//안정화모드 데이터 개수,1Cycle당
int NORM_TIMEOUT = 30;//안정화모드 타임아웃,초
double TOLERANCE = 0.5;//안정화모드 무게 차이 허용치,KG
int TOLERANCE_ALLOWABLE_VAL = 10;//안정화 되었다고 판단하는 허용치 내 데이터 개수
double NET_WEIGHT = 0.3; //뜰채 무게,KG
//↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑USER_PARAM
//---------------------------------------------------------
void setup()
{
  digitalWrite(RST_HWPIN, HIGH);
  delay(50);
  mcp.begin_I2C();
  pinMode(RST_HWPIN, OUTPUT);
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
  Serial.begin(4800);//Front Panel
  Serial.setTimeout(50);
  PC.begin(9600);//HC-12
  mcp.digitalWrite(SET, HIGH);
  scale.begin(HX711_DAT, HX711_CLK);
  scale.set_scale(CAL_FACTOR);
  scale.tare();
  datasend("READY", "0", "0");
  Serial.println("READY");
  delay(1500);
  mcp.digitalWrite(USB_POWER, HIGH);
}

void errchk(String RFID, double avg)
{
  if (avg < 0.09)
  {
    Serial.println("ERROR");
    datasend(RFID, "0.000", "1");
    digitalWrite(RST_HWPIN, LOW);
  }
  if (avg > 10)
  {
    Serial.println("ERROR");
    datasend(RFID, "0.000", "1");
    digitalWrite(RST_HWPIN, LOW);
  }
  return;
}

void datasend(String RFID, String Data1, String Data2)
{
  String DATA = RFID + "|" + Data1 + "|" + Data2 + "|" + EXT_BTN_DATA;
  PC.listen();
  PC.println(DATA);
  return;
}

String rfidparse()
{
  char RFID_CHAR[10];
  for (int i = 2; i <= 11; i++)
  {
    RFID_CHAR[i - 2] = UART_DATA.charAt(i);
  }
  String s1(RFID_CHAR[0]);
  String s2(RFID_CHAR[1]);
  String s3(RFID_CHAR[2]);
  String s4(RFID_CHAR[3]);
  String s5(RFID_CHAR[4]);
  String s6(RFID_CHAR[5]);
  String s7(RFID_CHAR[6]);
  String s8(RFID_CHAR[7]);
  String s9(RFID_CHAR[8]);
  String s10(RFID_CHAR[9]);
  String rfid = s1 + s2 + s3 + s4 + s5 + s6 + s7 + s8 + s9 + s10;
  return rfid;
}

String uartparse()
{
  char alpha = ((UART_DATA.charAt(0)));
  char bravo = ((UART_DATA.charAt(1)));
  String s1(alpha);
  String s2(bravo);
  String rfid = s1 + s2;
  return rfid;
}

void serialEvent()
{
  UART_DATA = Serial.readString();
  UART_SIGN = uartparse();
}

void loop()
{
  if (UART_SIGN == "TR")
  {
    scale.tare();
    mcp.digitalWrite(USB_POWER, LOW);
    delay(200);
    mcp.digitalWrite(USB_POWER, HIGH);
    UART_SIGN = "  ";
  }
  if (UART_SIGN == "RF")
  {
    UART_SIGN = "  ";
    mcp.digitalWrite(USB_POWER, LOW);
    RFID_DATA = rfidparse();
    datasend(RFID_DATA, "0", "0");
    delay(1000);
    unsigned long WAIT_PRV_MILLIS = millis();
    while (millis() < WAIT_PRV_MILLIS + (INPUT_TIMEOUT * 1000) - 1000)
    {
      if (scale.get_units(10) > 0.1)
      {
        if (mcp.digitalRead(SCALE_MODE) == HIGH)
        {
          FAIL:
          MODE_FLAG = 0;
          double READ_DATA[AVERAGE_MODE_DATA_AMOUNT];
          double AVG_DATA = 0;
          for (int i = 0; i < AVERAGE_MODE_DATA_AMOUNT; i++)
          {
            double RAW_DATA = scale.get_units(2);
            if (RAW_DATA > 0.09 && RAW_DATA < 10)
            {
              READ_DATA[i] = RAW_DATA;
              //PC.println(READ_DATA[i]);
            }
            else
            {
              READ_DATA[i] = READ_DATA[i - 1];
            }
          }
          for (int i = 0; i < AVERAGE_MODE_DATA_AMOUNT; i++)
          {
            AVG_DATA = AVG_DATA + READ_DATA[i];
            //PC.println(AVG_DATA);
          }
          AVG_DATA = AVG_DATA / AVERAGE_MODE_DATA_AMOUNT;
          Serial.println("BTNDT");
          goto btnpoint;
          AVERAGE:
          if (NET_FLAG == 1)
          {
            AVG_DATA = AVG_DATA - NET_WEIGHT;
          }
          errchk(RFID_DATA, AVG_DATA);
          String AVG_DATA_STR = String(AVG_DATA, 3);
          Serial.println(AVG_DATA_STR);
          datasend(RFID_DATA, AVG_DATA_STR, "0");
          mcp.digitalWrite(MOTOR_CONTROL, HIGH);
          delay(100);
          mcp.digitalWrite(MOTOR_CONTROL, LOW);
          delay(8000);
          digitalWrite(RST_HWPIN, LOW);
        }
//---------------------------------------------------------
        if (mcp.digitalRead(SCALE_MODE) == LOW)
        {
          MODE_FLAG = 1;
          unsigned long NORM_PRV_MILLIS = millis();
          while (millis() < NORM_PRV_MILLIS + NORM_TIMEOUT*1000)
          {
            double READ_DATA[STBL_MODE_DATA_AMOUNT] = {0};
            double DIFF_ARR[STBL_MODE_DATA_AMOUNT - 1] = {0};
            double AVG_DATA = 0;
            int TOLERANCE_ALLOWABLE_DATA = 0;
            for (int i = 0; i < STBL_MODE_DATA_AMOUNT; i++)
            {
              double RAW_DATA = scale.get_units(2);
              if (RAW_DATA < 10.000 && RAW_DATA > 0.09)
              {
                READ_DATA[i] = RAW_DATA;
                //PC.println(READ_DATA[i]);
              }
              else
              {
                 READ_DATA[i] = READ_DATA[i-1];
              }
            }
            for (int i = 0; i < STBL_MODE_DATA_AMOUNT - 1; i++)
            {
              float temp = READ_DATA[i] - READ_DATA[i + 1];
              DIFF_ARR[i] = abs(temp);
              //PC.println(DIFF_ARR[i]);
            }
            for (int i = 0; i < STBL_MODE_DATA_AMOUNT - 1; i++)
            {
              if (DIFF_ARR[i] < TOLERANCE)
              {
                AVG_DATA = AVG_DATA + READ_DATA[i] + READ_DATA[i + 1];
                TOLERANCE_ALLOWABLE_DATA++;
                //PC.println(AVG_DATA);
                //PC.println(TOLERANCE_ALLOWABLE_DATA);
              }
            }
            if (TOLERANCE_ALLOWABLE_DATA >= TOLERANCE_ALLOWABLE_VAL)
            {
              Serial.println("BTNDT");
              goto btnpoint;
              STBL:
              double AVG_DATA_FINAL = AVG_DATA / (TOLERANCE_ALLOWABLE_DATA * 2);
              if (NET_FLAG == 1)
              {
                AVG_DATA_FINAL = AVG_DATA_FINAL - NET_WEIGHT;
              }
              errchk(RFID_DATA, AVG_DATA_FINAL);
              String AVG_DATA_STR = String(AVG_DATA_FINAL, 3);
              Serial.println(AVG_DATA_STR);
              datasend(RFID_DATA, AVG_DATA_STR, "0");
              mcp.digitalWrite(MOTOR_CONTROL, HIGH);
              delay(100);
              mcp.digitalWrite(MOTOR_CONTROL, LOW);
              delay(8000);
              digitalWrite(RST_HWPIN, LOW);
            }
          }
          goto FAIL;
        }
      }
    }
    Serial.println("TIMEO");
    datasend(RFID_DATA, "0.000", "1");
    digitalWrite(RST_HWPIN, LOW);
  }
btnpoint:
  if (UART_SIGN == "01")
  {
    EXT_BTN_DATA = 1;
    if(MODE_FLAG == 0)
    {
      goto AVERAGE;
    }
    else
    {
      goto STBL;
    }
  }
  if (UART_SIGN == "02")
  {
    EXT_BTN_DATA = 2;
    if(MODE_FLAG == 0)
    {
      goto AVERAGE;
    }
    else
    {
      goto STBL;
    }
  }
  if (UART_SIGN == "03")
  {
    EXT_BTN_DATA = 3;
    if(MODE_FLAG == 0)
    {
      goto AVERAGE;
    }
    else
    {
      goto STBL;
    }
  }
  if (UART_SIGN == "04")
  {
    EXT_BTN_DATA = 4;
    if(MODE_FLAG == 0)
    {
      goto AVERAGE;
    }
    else
    {
      goto STBL;
    }
  }
  if (UART_SIGN == "11")
  {
    NET_FLAG = 1;
    EXT_BTN_DATA = 1;
    if(MODE_FLAG == 0)
    {
      goto AVERAGE;
    }
    else
    {
      goto STBL;
    }
  }
  if (UART_SIGN == "12")
  {
    NET_FLAG = 1;
    EXT_BTN_DATA = 2;
    if(MODE_FLAG == 0)
    {
      goto AVERAGE;
    }
    else
    {
      goto STBL;
    }
  }
  if (UART_SIGN == "13")
  {
    NET_FLAG = 1;
    EXT_BTN_DATA = 3;
    if(MODE_FLAG == 0)
    {
      goto AVERAGE;
    }
    else
    {
      goto STBL;
    }
  }
  if (UART_SIGN == "14")
  {
    NET_FLAG = 1;
    EXT_BTN_DATA = 4;
    if(MODE_FLAG == 0)
    {
      goto AVERAGE;
    }
    else
    {
      goto STBL;
    }
  }
  if (UART_SIGN == "10")
  {
    NET_FLAG = 1;
    if(MODE_FLAG == 0)
    {
      goto AVERAGE;
    }
    else
    {
      goto STBL;
    }
  }
  if (UART_SIGN == "00")
  {
    if(MODE_FLAG == 0)
    {
      goto AVERAGE;
    }
    else
    {
      goto STBL;
    }
  }
}
