#include <Adafruit_MCP23X17.h>
#include <SoftwareSerial.h>
#include "HX711.h"
#include <Wire.h>
#include <EEPROM.h>
// EEPROM 0:Callibrate Value

//-----------------------------------------------IO PIN
#define HX711_DAT 4
#define HX711_CLK 5
#define PC_RX 8
#define PC_TX 7
#define RST_HWPIN 9
//-----------------------------------------------MCP PIN
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

//---------------------------------------------------------
// ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓USER_PARAM
int INPUT_TIMEOUT = 10;            // 투입 타임아웃,초
int AVERAGE_MODE_DATA_AMOUNT = 35; // 평균모드 데이터 개수,시간 계산법:35*2/10 = 7초
int STBL_MODE_DATA_AMOUNT = 15;    // 안정화모드 데이터 개수,1Cycle당
int NORM_TIMEOUT = 30;             // 안정화모드 타임아웃,초
double TOLERANCE = 0.5;            // 안정화모드 무게 차이 허용치,KG
int TOLERANCE_ALLOWABLE_VAL = 10;  // 안정화 되었다고 판단하는 허용치 내 데이터 개수
double NET_WEIGHT = 0.3;           // 뜰채 무게,KG
double CAL_WEIGHT = 0.163;         // 캘리브레이션 시 투입 무게
// ↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑USER_PARAM
//---------------------------------------------------------

unsigned char uart_buffer[30] = {0x00};
bool UARTeventFlag = 0;
bool initFlag = 0;
bool rfidFlag = 0;
bool measureFlag = 0;
bool measureEndFlag = 0;
unsigned char RFID_DATA[10] = {0x00};
unsigned char ETC_PARAM[5] = {0x00};
float CAL_VALUE = 0.0;
double weight = 0.0;

Adafruit_MCP23X17 mcp;
HX711 scale;
SoftwareSerial PC(PC_RX, PC_TX);

void setup()
{
  digitalWrite(RST_HWPIN, HIGH);
  delay(50);
  pinMode(RST_HWPIN, OUTPUT);

  //---------------------------------------------------------------mcp.begin_I2C();
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

  EEPROM.get(0, CAL_VALUE);
  scale.begin(HX711_DAT, HX711_CLK);
  scale.set_scale(CAL_VALUE);
  scale.tare();
  unsigned char init_message[4] = {0x02, 0x05, 0x03, 0x17};
  delay(3000);
  Serial.write(init_message, sizeof(init_message));
  initFlag = 1;
}

void serialEvent()
{
  // digitalWrite(13, HIGH);
  Serial.readBytesUntil(0x17, uart_buffer, 30);
  // Serial.write(uart_buffer, 20);
  UARTeventFlag = 1;
}

void motorControl()
{
  mcp.digitalWrite(MOTOR_CONTROL, HIGH);
  delay(100);
  mcp.digitalWrite(MOTOR_CONTROL, LOW);
  delay(8000);
  mcp.digitalWrite(MOTOR_CONTROL, HIGH);
}

void callibrateSequence()
{
  unsigned char cal1_message[33] = {0x02, 0x19, 'C', 'A', 'L', 'L', 'I', 'B', 'R', 'A', 'T', 'E', 0x20, 'M', 'O', 'D', 'E', 0x20, 'D', 'O', 0x20, 'N', 'O', 'T', 0x20, 'T', 'O', 'U', 'C', 'H', '!', 0x03, 0x17};
  Serial.write(cal1_message, sizeof(cal1_message));
  delay(2500);
  // Don't touch.
  scale.set_scale();
  scale.tare();
  // Place known value-1.00kg
  // Don't touch...
  unsigned char cal2_message[28] = {0x02, 0x19, 'P', 'L', 'A', 'C', 'E', 0x20, 'A', 0x20, 'W', 'E', 'I', 'G', 'H', 'T', 0x20, 0x20, '-', '0', '.', '0', '0', '0', 'K', 'G', 0x03, 0x17};
  unsigned char weight_char[6];
  dtostrf(CAL_WEIGHT, 5, 3, weight_char);
  for (int i = 19; i < 24; i++)
  {
    cal2_message[i] = weight_char[i - 19];
  }
  Serial.write(cal2_message, sizeof(cal2_message));
  delay(5000);
  for (int i = 5; i > 0; i--)
  {
    unsigned char cal3_message[5] = {0x02, 0x19, i + 48, 0x03, 0x17};
    Serial.write(cal3_message, sizeof(cal3_message));
    delay(1000);
  }
  float scale_cal_raw = scale.get_units(10);
  CAL_VALUE = scale_cal_raw / CAL_WEIGHT;
  // Finish
  EEPROM.put(0, CAL_VALUE);
  unsigned char cal_val_char[9];
  dtostrf(CAL_VALUE, 8, 1, cal_val_char);
  unsigned char cal4_message[28] = {0x02, 0x19, 'F', 'I', 'N', 'I', 'S', 'H', 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, '0', '0', '0', '0', '0', '0', '.', '0', 0x03, 0x17};
  for (int i = 18; i < 26; i++)
  {
    cal4_message[i] = cal_val_char[i - 18];
  }
  Serial.write(cal4_message, sizeof(cal4_message));
  delay(1500);
}

double averageMode()
{
  double READ_DATA[AVERAGE_MODE_DATA_AMOUNT];
  double AVG_DATA = 0;
  for (int i = 0; i < AVERAGE_MODE_DATA_AMOUNT; i++)
  {
    if (scale.is_ready())
    {
      double RAW_DATA = scale.get_units(2);
      if (RAW_DATA > 0.09 && RAW_DATA < 10)
      {
        READ_DATA[i] = RAW_DATA;
        // PC.println(READ_DATA[i]);
      }
      else
      {
        READ_DATA[i] = READ_DATA[i - 1];
      }
    }
    else
    {
      i--;
    }
    if (i < -1)
    {
      return -1;
    }
  }
  for (int i = 0; i < AVERAGE_MODE_DATA_AMOUNT; i++)
  {
    AVG_DATA = AVG_DATA + READ_DATA[i];
    // PC.println(AVG_DATA);
  }
  AVG_DATA = AVG_DATA / AVERAGE_MODE_DATA_AMOUNT;
  return AVG_DATA;
}

double stableMode()
{
  unsigned long NORM_PRV_MILLIS = millis();
  while (millis() < NORM_PRV_MILLIS + NORM_TIMEOUT * 1000)
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
        // PC.println(READ_DATA[i]);
      }
      else
      {
        READ_DATA[i] = READ_DATA[i - 1];
      }
    }
    for (int i = 0; i < STBL_MODE_DATA_AMOUNT - 1; i++)
    {
      float temp = READ_DATA[i] - READ_DATA[i + 1];
      DIFF_ARR[i] = abs(temp);
      // PC.println(DIFF_ARR[i]);
    }
    for (int i = 0; i < STBL_MODE_DATA_AMOUNT - 1; i++)
    {
      if (DIFF_ARR[i] < TOLERANCE)
      {
        AVG_DATA = AVG_DATA + READ_DATA[i] + READ_DATA[i + 1];
        TOLERANCE_ALLOWABLE_DATA++;
        // PC.println(AVG_DATA);
        // PC.println(TOLERANCE_ALLOWABLE_DATA);
      }
    }
    if (TOLERANCE_ALLOWABLE_DATA >= TOLERANCE_ALLOWABLE_VAL)
    {
      double AVG_DATA_FINAL = AVG_DATA / (TOLERANCE_ALLOWABLE_DATA * 2);
      return AVG_DATA_FINAL;
      break;
    }
  }
  return -1;
}

void loop()
{
  if (UARTeventFlag == 1)
  {
    if (uart_buffer[1] == 0x06) // ACK
    {
      if (initFlag == 1)
      {
        mcp.digitalWrite(USB_POWER, HIGH);
        initFlag = 0;
        rfidFlag = 0;
        measureFlag = 0;
        measureEndFlag = 0;
        RFID_DATA[10] = {0x00};
        ETC_PARAM[5] = {0x00};
        weight = 0.0;
        unsigned char pc_init_message[13] = {'R', 'E', 'A', 'D', 'Y', '|', '0', '|', '0', '|', '0', 0x0D, 0x0A};
        PC.listen();
        PC.write(pc_init_message, sizeof(pc_init_message));
      }
      uart_buffer[30] = {0x00};
    }

    if (uart_buffer[1] == 0x11) // RFID Data
    {
      for (int i = 0; i < 10; i++)
      {
        RFID_DATA[i] = uart_buffer[i + 2];
      }
      unsigned char ack_message[4] = {0x02, 0x06, 0x03, 0x17};
      Serial.write(ack_message, sizeof(ack_message));
      rfidFlag = 1;
      uart_buffer[30] = {0x00};
    }

    if (uart_buffer[1] == 0x13 && measureEndFlag == 1) // Send Data Signal
    {
      measureEndFlag = 0;
      if (uart_buffer[2] == 0xFF)
      {
        weight = weight - NET_WEIGHT;
      }
      if (weight < 0.05 || weight > 10.0) // 예외처리
      {
        unsigned char rangeout_message[29] = {0x02, 0x19, 'E', 'R', 'R', 'O', 'R', 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 'R', 'A', 'N', 'G', 'E', 0x20, 'O', 'U', 'T', 0x03, 0x17};
        Serial.write(rangeout_message, sizeof(rangeout_message));
        unsigned char pc_message[22] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '.', '0', '0', '0', '|', '1', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0.000|0|0CRLF
        PC.listen();
        PC.write(pc_message, sizeof(pc_message));
        for (int i = 0; i < 10; i++)
        {
          pc_message[i] = RFID_DATA[i];
        }
      }
      else
      {
        unsigned char weight_char[6];
        dtostrf(weight, 5, 3, weight_char);
        unsigned char clcd_message[11] = {0x02, 0x19, '0', '.', '0', '0', '0', 'K', 'G', 0x03, 0x17};
        for (int i = 0; i < 5; i++)
        {
          clcd_message[i + 2] = weight_char[i];
        }
        Serial.write(clcd_message, sizeof(clcd_message));
        unsigned char pc_message[22] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '.', '0', '0', '0', '|', '0', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0.000|0|0CRLF
        for (int i = 0; i < 10; i++)
        {
          pc_message[i] = RFID_DATA[i];
        }
        for (int i = 11; i < 16; i++)
        {
          pc_message[i] = weight_char[i - 11];
        }
        pc_message[19] = uart_buffer[3];
        PC.listen();
        PC.write(pc_message, sizeof(pc_message));
        motorControl();
      }
      unsigned char init_message[4] = {0x02, 0x05, 0x03, 0x17};
      Serial.write(init_message, sizeof(init_message));
      initFlag = 1;
    }

    if (uart_buffer[1] == 0x14) // Callibrate
    {
      callibrateSequence();
      initFlag = 1;
      unsigned char init_message[4] = {0x02, 0x05, 0x03, 0x17};
      Serial.write(init_message, sizeof(init_message));
      uart_buffer[30] = {0x00};
    }

    if (uart_buffer[1] == 0x16) // Tare
    {
      scale.tare();
      mcp.digitalWrite(USB_POWER, LOW);
      delay(200);
      mcp.digitalWrite(USB_POWER, HIGH);
      uart_buffer[30] = {0x00};
    }

    if (uart_buffer[1] == 0x15) // NAK
    {
    }
    UARTeventFlag = 0;
  }

  if (rfidFlag == 1) // 측량 시작
  {
    mcp.digitalWrite(USB_POWER, LOW);
    unsigned char pc_message[18] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '|', '0', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0|0|0CRLF
    for (int i = 0; i < 10; i++)
    {
      pc_message[i] = RFID_DATA[i];
    }
    PC.listen();
    PC.write(pc_message, sizeof(pc_message));
    unsigned char measuring_message[14] = {0x02, 0x19, 'M', 'E', 'A', 'S', 'U', 'R', 'I', 'N', 'G', 0x03, 0x17};
    Serial.write(measuring_message, sizeof(measuring_message));
    unsigned long WAIT_PRV_MILLIS = millis();
    while (1)
    {
      if (millis() >= WAIT_PRV_MILLIS + (INPUT_TIMEOUT * 1000) - 1000) // 투입 타임아웃
      {
        unsigned char timeout_message[11] = {0x02, 0x19, 'T', 'I', 'M', 'E', 'O', 'U', 'T', 0x03, 0x17};
        Serial.write(timeout_message, sizeof(timeout_message));
        unsigned char pc_timeout_message[22] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '.', '0', '0', '0', '|', '1', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0.000|1|0
        for (int i = 0; i < 10; i++)
        {
          pc_timeout_message[i] = RFID_DATA[i];
        }
        PC.listen();
        PC.write(pc_timeout_message, sizeof(pc_timeout_message));
        rfidFlag = 0;
        initFlag = 1;
        delay(5000);
        unsigned char init_message[4] = {0x02, 0x05, 0x03, 0x17};
        Serial.write(init_message, sizeof(init_message));
        break;
      }

      if (scale.get_units(15) > 0.1) // 무게 감지
      {
        measureFlag = 1;
        break;
      }
    }
  }

  if (rfidFlag == 1 && measureFlag == 1) // 실제 무게 측량 처리
  {
    if (mcp.digitalRead(SCALE_MODE) == LOW) // 평균 모드
    {
      unsigned char mode_message[23] = {0x02, 0x19, 'M', 'E', 'A', 'S', 'U', 'R', 'I', 'N', 'G', 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 'A', 'V', 'G', 0x03, 0x17};
      Serial.write(mode_message, sizeof(mode_message));
      weight = averageMode();
      if (weight != -1) // 정상
      {
        unsigned char datareq_message[4] = {0x02, 0x12, 0x03, 0x17};
        Serial.write(datareq_message, sizeof(datareq_message));
        measureEndFlag = 1;
      }
      else if (weight == -1) // 실패
      {
        unsigned char fail_message[10] = {0x02, 0x19, 'E', 'R', 'R', 'O', 'R', 0x03, 0x17};
        Serial.write(fail_message, sizeof(fail_message));
        delay(3000);
        initFlag = 1;
        unsigned char init_message[4] = {0x02, 0x05, 0x03, 0x17};
        Serial.write(init_message, sizeof(init_message));
      }
    }
    else if (mcp.digitalRead(SCALE_MODE) == HIGH) // 안정화 모드
    {
      unsigned char mode_message[24] = {0x02, 0x19, 'M', 'E', 'A', 'S', 'U', 'R', 'I', 'N', 'G', 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 0x20, 'S', 'T', 'B', 'L', 0x03, 0x17};
      Serial.write(mode_message, sizeof(mode_message));
      double weight_stbl = stableMode();
      if (weight_stbl != -1) // 정상
      {
        weight = weight_stbl;
        unsigned char datareq_message[4] = {0x02, 0x12, 0x03, 0x17};
        Serial.write(datareq_message, sizeof(datareq_message));
        measureEndFlag = 1;
      }
      else if (weight_stbl == -1) // 실패
      {
        weight = averageMode(); // 재시도
        if (weight != -1)       // 정상
        {
          unsigned char datareq_message[4] = {0x02, 0x12, 0x03, 0x17};
          Serial.write(datareq_message, sizeof(datareq_message));
          measureEndFlag = 1;
        }
        else if (weight == -1) // 실패
        {
          unsigned char fail_message[10] = {0x02, 0x19, 'E', 'R', 'R', 'O', 'R', 0x03, 0x17};
          Serial.write(fail_message, sizeof(fail_message));
          delay(3000);
          initFlag = 1;
          unsigned char init_message[4] = {0x02, 0x05, 0x03, 0x17};
          Serial.write(init_message, sizeof(init_message));
        }
      }
    }
    rfidFlag = 0;
    measureFlag = 0;
  }
}
