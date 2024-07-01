#include <SoftwareSerial.h>
#include "HX711.h"
#include <EEPROM.h>

// GPIO
#define RFID_TX 2
#define RFID_RX 3
#define PC_TX 4
#define PC_RX 5
#define HX711_DAT 6
#define HX711_CLK 7
#define RELAY_1 8
#define RELAY_2 9
#define USB_POWER 10
#define RST_HWPIN 11
#define SCALE_MODE 12
#define NET_SENSOR A0
#define TARE A1

//---------------------------------------------------------
// ↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓USER_PARAM
int INPUT_TIMEOUT = 15;             // 투입 타임아웃,초
int AVERAGE_MODE_DATA_AMOUNT = 35; // 평균모드 데이터 개수,시간 계산법:35*2/10 = 7초
int STBL_MODE_DATA_AMOUNT = 15;    // 안정화모드 데이터 개수,1Cycle당
int NORM_TIMEOUT = 30;              // 안정화모드 타임아웃,초
double TOLERANCE = 0.5;             // 안정화모드 무게 차이 허용치,KG
int TOLERANCE_ALLOWABLE_VAL = 80;   // 안정화 되었다고 판단하는 허용치 내 데이터 개수
double NET_WEIGHT = 0.3;            // 뜰채 무게,KG
double CAL_WEIGHT = 0.410;          // 캘리브레이션 시 투입 무게
// ↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑USER_PARAM
//---------------------------------------------------------

bool initFlag = false;
bool RFIDeventFlag = false;
bool measureFlag = false;
bool measureEndFlag = false;
unsigned char rfid_buffer[15] = {0x00};
unsigned char RFID_DATA[10] = {0x00};
float CAL_VALUE = -77800.0;
double weight = 0.0;

HX711 scale;
SoftwareSerial rfid(RFID_RX, RFID_TX);
SoftwareSerial PC(PC_RX, PC_TX);

void motor(int mdelay) // 모터 구동
{
  digitalWrite(RELAY_1, HIGH);
  digitalWrite(RELAY_2, LOW);
  delay(mdelay);
  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, HIGH);
  delay(mdelay);
  digitalWrite(RELAY_1, LOW);
  digitalWrite(RELAY_2, LOW);
  return;
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
        //PC.println(READ_DATA[i]);
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
    //PC.println(AVG_DATA);
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
        //PC.println(READ_DATA[i]);
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

void callibrateSequence(double known_weight)
{
  rfid.listen();
  rfid.println("CALLIBRATE MODE DO NOT TOUCH!");
  delay(2500);
  // Don't touch.
  scale.set_scale();
  scale.tare();
  // Place known value-1.00kg
  // Don't touch...
  rfid.print("PLACE A WEIGHT = ");
  rfid.print(known_weight);
  rfid.println("KG");
  delay(2000);
  for (int i = 5; i > 0; i--)
  {
    rfid.println(i);
    delay(1000);
  }
  float scale_cal_raw = scale.get_units(10);
  CAL_VALUE = scale_cal_raw / known_weight;
  // Finish
  EEPROM.put(0, CAL_VALUE);
  rfid.print("FINISH ");
  rfid.println(CAL_VALUE);
  delay(1500);
}

void setup()
{
  pinMode(RELAY_1, OUTPUT);
  pinMode(RELAY_2, OUTPUT);
  pinMode(USB_POWER, OUTPUT);
  pinMode(SCALE_MODE, INPUT_PULLUP);
  pinMode(TARE, INPUT_PULLUP);
  Serial.begin(9600);
  rfid.begin(9600);
  PC.begin(300);
  EEPROM.get(0, CAL_VALUE);
  scale.begin(HX711_DAT, HX711_CLK);
  initFlag = true;
}

void loop()
{
  if (digitalRead(TARE) == LOW) // 영점버튼
  {
    scale.tare();
    digitalWrite(USB_POWER, LOW);
    delay(200);
    digitalWrite(USB_POWER, HIGH);
  }

  if (initFlag == true)
  {
    initFlag = false;
    RFIDeventFlag = false;
    measureFlag = false;
    measureEndFlag = false;
    weight = 0.0;
    rfid_buffer[15] = {0x00};
    RFID_DATA[10] = {0x00};
    scale.set_scale(CAL_VALUE);
    scale.tare();
    digitalWrite(USB_POWER, HIGH);
    unsigned char pc_init_message[13] = {'R', 'E', 'A', 'D', 'Y', '|', '0', '|', '0', '|', '0', 0x0D, 0x0A};
    PC.listen();
    PC.write(pc_init_message, sizeof(pc_init_message));
    //PC.println(CAL_VALUE);
    rfid.listen();
  }

  if (rfid.available())
  {
    rfid.readBytesUntil(0x03, rfid_buffer, 14);
    if (rfid_buffer[0] == 0x02 && rfid_buffer[1] == 0x02)
    {
      if (rfid_buffer[2] == 0x11) // Callibrate 진행
      {
        digitalWrite(USB_POWER, LOW);
        double known_weight = ((rfid_buffer[3] - '0') * 1000) + ((rfid_buffer[4] - '0') * 100) + ((rfid_buffer[5] - '0') * 10) + (rfid_buffer[6] - '0');
        known_weight = known_weight / 1000;
        callibrateSequence(known_weight);
        initFlag = true;
      }
      if (rfid_buffer[2] == 0x12) // Callibrate 값 확인
      {
        rfid.println(CAL_VALUE);
      }
    }
    else
    {
      for (int i = 0; i < 10; i++)
      {
        RFID_DATA[i] = rfid_buffer[i + 1];
      }
      unsigned char pc_message[18] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '|', '0', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0|0|0CRLF
      for (int i = 0; i < 10; i++)
      {
        pc_message[i] = RFID_DATA[i];
      }
      PC.listen();
      PC.write(pc_message, sizeof(pc_message));
      digitalWrite(USB_POWER, LOW);
      delay(1000);
      RFIDeventFlag = true;
    }
  }

  if (RFIDeventFlag == true)
  {
    unsigned long WAIT_PRV_MILLIS = millis();
    while (1)
    {
      if (millis() >= WAIT_PRV_MILLIS + (INPUT_TIMEOUT * 1000) - 1000) // 투입 타임아웃
      {
        unsigned char pc_timeout_message[22] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '.', '0', '0', '0', '|', '1', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0.000|1|0
        for (int i = 0; i < 10; i++)
        {
          pc_timeout_message[i] = RFID_DATA[i];
        }
        PC.listen();
        PC.write(pc_timeout_message, sizeof(pc_timeout_message));
        RFIDeventFlag = false;
        initFlag = true;
        delay(5000);
        break;
      }

      if (scale.get_units(15) > 0.1) // 무게 감지
      {
        measureFlag = true;
        break;
      }
    }
  }

  if (RFIDeventFlag == true && measureFlag == true) // 실제 무게 측량 처리
  {
    if (digitalRead(SCALE_MODE) == HIGH) // 평균 모드
    {
      weight = averageMode();
      if (weight != -1) // 정상
      {
        measureEndFlag = true;
      }
      else if (weight == -1) // 실패
      {
        unsigned char measure_err_message[22] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '.', '0', '0', '0', '|', '1', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0.000|1|0
        for (int i = 0; i < 10; i++)
        {
          measure_err_message[i] = RFID_DATA[i];
        }
        PC.listen();
        PC.write(measure_err_message, sizeof(measure_err_message));
        delay(3000);
        initFlag = true;
      }
    }
    else if (digitalRead(SCALE_MODE) == LOW) // 안정화 모드
    {
      double weight_stbl = stableMode();
      if (weight_stbl != -1) // 정상
      {
        weight = weight_stbl;
        measureEndFlag = true;
      }
      else if (weight_stbl == -1) // 실패
      {
        weight = averageMode(); // 재시도
        if (weight != -1)       // 정상
        {
          measureEndFlag = true;
        }
        else if (weight == -1) // 실패
        {
          unsigned char measure_err_message[22] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '.', '0', '0', '0', '|', '1', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0.000|1|0
          for (int i = 0; i < 10; i++)
          {
            measure_err_message[i] = RFID_DATA[i];
          }
          PC.listen();
          PC.write(measure_err_message, sizeof(measure_err_message));
          delay(3000);
          initFlag = true;
        }
      }
    }
    RFIDeventFlag = false;
    measureFlag = false;
  }

  if (measureEndFlag == true) // Send Data Signal
  {
    measureEndFlag = false;
    if (digitalRead(NET_SENSOR) == LOW)
    {
      weight = weight - NET_WEIGHT;
    }
    if (weight < 0.05 || weight > 10.0) // 예외처리
    {
      unsigned char pc_message[22] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '.', '0', '0', '0', '|', '1', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0.000|0|0CRLF
      for (int i = 0; i < 10; i++)
      {
        pc_message[i] = RFID_DATA[i];
      }
      PC.listen();
      PC.write(pc_message, sizeof(pc_message));
    }
    else
    {
      unsigned char weight_char[6];
      dtostrf(weight, 5, 3, weight_char);
      unsigned char pc_message[22] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '.', '0', '0', '0', '|', '0', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0.000|0|0CRLF
      for (int i = 0; i < 10; i++)
      {
        pc_message[i] = RFID_DATA[i];
      }
      for (int i = 11; i < 16; i++)
      {
        pc_message[i] = weight_char[i - 11];
      }
      PC.listen();
      PC.write(pc_message, sizeof(pc_message));
      motor(3100);
    }
    initFlag = true;
  }
}