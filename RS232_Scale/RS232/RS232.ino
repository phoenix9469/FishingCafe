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
int INPUT_TIMEOUT = 10;            // 투입 타임아웃,초
int AVERAGE_MODE_DATA_AMOUNT = 35; // 평균모드 데이터 개수,시간 계산법:35*2/10 = 7초
int STBL_MODE_DATA_AMOUNT = 15;    // 안정화모드 데이터 개수,1Cycle당
int NORM_TIMEOUT = 30;             // 안정화모드 타임아웃,초
double TOLERANCE = 0.5;            // 안정화모드 무게 차이 허용치,KG
int TOLERANCE_ALLOWABLE_VAL = 10;  // 안정화 되었다고 판단하는 허용치 내 데이터 개수
double NET_WEIGHT = 0.3;           // 뜰채 무게,KG
double CAL_WEIGHT = 0.410;         // 캘리브레이션 시 투입 무게
// ↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑USER_PARAM
//---------------------------------------------------------

bool initFlag = 0;
bool RFIDeventFlag = 0;
unsigned char rfid_buffer[12] = {0x00};
unsigned char RFID_DATA[10] = {0x00};
float CAL_VALUE = 0.0;
double weight = 0.0;

HX711 scale;
SoftwareSerial rfid(RFID_RX, RFID_TX);
SoftwareSerial PC(PC_RX, PC_TX);

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
    scale.set_scale(CAL_VALUE);
    scale.tare();
    initFlag = 1;
}

void loop()
{
    if(initFlag == 1)
    {
        weight = 0.0;
        rfid_buffer[12] = {0x00};
        RFID_DATA[10] = {0x00};
        digitalWrite(USB_POWER, HIGH);
        unsigned char pc_init_message[13] = {'R', 'E', 'A', 'D', 'Y', '|', '0', '|', '0', '|', '0', 0x0D, 0x0A};
        PC.listen();
        PC.write(pc_init_message, sizeof(pc_init_message));
        initFlag == 0;
    }

    if (rfid.available())
    {
        rfid.readBytes(rfid_buffer, 11);
        for (int i = 0; i < 10; i++)
        {
            RFID_DATA[i] = rfid_buffer[i + 1];
        }
        RFIDeventFlag = 1;
    }

    if(RFIDeventFlag == 1)
    {
        
    }
}