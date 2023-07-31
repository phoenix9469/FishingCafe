#include <SoftwareSerial.h>
#include "HX711.h"
// ---------------------핀 지정
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
//↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓↓USER_PARAM
int INPUT_TIMEOUT = 10;            //투입 타임아웃,초
int AVERAGE_MODE_DATA_AMOUNT = 280; //평균모드 데이터 개수,시간 계산법:35*2/10 = 7초
int STBL_MODE_DATA_AMOUNT = 120;    //안정화모드 데이터 개수,1Cycle당
int NORM_TIMEOUT = 30;             //안정화모드 타임아웃,초
double TOLERANCE = 0.5;            //안정화모드 무게 차이 허용치,KG
int TOLERANCE_ALLOWABLE_VAL = 80;  //안정화 되었다고 판단하는 허용치 내 데이터 개수
double NET_WEIGHT = 0.3;           //뜰채 무게,KG
//↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑↑USER_PARAM
//---------------------------------------------------------
#define calibration_factor -77800.0 //1:-81047.0 2:77800 3:82500
HX711 scale;
SoftwareSerial RFID(RFID_RX, RFID_TX); // RX, TX
SoftwareSerial PC(PC_RX, PC_TX);       // RX, TX

void setup()
{
    digitalWrite(RST_HWPIN, HIGH);
    delay(50);
    pinMode(RELAY_1, OUTPUT);
    pinMode(RELAY_2, OUTPUT);
    pinMode(USB_POWER, OUTPUT);
    pinMode(RST_HWPIN, OUTPUT);
    pinMode(SCALE_MODE, INPUT_PULLUP);
    pinMode(TARE, INPUT_PULLUP);
    Serial.begin(9600);
    RFID.begin(9600);
    PC.begin(300);
    scale.begin(HX711_DAT, HX711_CLK);
    scale.set_scale(calibration_factor);
    scale.tare();
    datasend("READY", "0", "0");
    digitalWrite(USB_POWER, HIGH);
}

void motor(int mdelay) //모터 구동
{
    digitalWrite(RELAY_1, HIGH);
    digitalWrite(RELAY_2, LOW);
    delay(mdelay);
    digitalWrite(RELAY_1, LOW);
    digitalWrite(RELAY_2, HIGH);
    delay(mdelay);
    digitalWrite(RST_HWPIN, LOW);
    return;
}

float errchk(String RFID, float avg)
{
    if (avg < 0.09) // avg 음수일 경우
    {
        datasend(RFID, "0.000", "1");
        digitalWrite(RST_HWPIN, LOW); //종료
    }
    if (avg > 10) // avg 10Kg 초과일 경우
    {
        datasend(RFID, "0.000", "1");
        digitalWrite(RST_HWPIN, LOW); //종료
    }
    if (digitalRead(NET_SENSOR) == LOW)
    {
        return avg - NET_WEIGHT;
    }
    else
    {
        return avg;
    }
}

void datasend(String RFID, String Data1, String Data2) // PC로 데이터 전송
{
    String Data3 = "0";
    String DATA = RFID + "|" + Data1 + "|" + Data2 + "|" + Data3;
    PC.listen();
    PC.println(DATA);
    return;
}

String rfidparse() // RFID 데이터 파싱
{
    String rfidread = RFID.readString();
    char alpha = ((rfidread.charAt(1)));
    char bravo = ((rfidread.charAt(2)));
    char charlie = ((rfidread.charAt(3)));
    char delta = ((rfidread.charAt(4)));
    char echo = ((rfidread.charAt(5)));
    char foxtrot = ((rfidread.charAt(6)));
    char golf ((rfidread.charAt(7)));
    char hotel ((rfidread.charAt(8)));
    char india ((rfidread.charAt(9)));
    char juliett ((rfidread.charAt(10)));
    String s1(alpha);
    String s2(bravo);
    String s3(charlie);
    String s4(delta);
    String s5(echo);
    String s6(foxtrot);
    String s7(golf);
    String s8(hotel);
    String s9(india);
    String s10(juliett);
    String rfid = s1+s2+s3+s4+s5+s6+s7+s8+s9+s10;
    return rfid;
}

void loop()
{
    RFID.listen();
    if (digitalRead(TARE) == LOW) // 영점버튼
    {
        scale.tare();
        digitalWrite(USB_POWER, LOW);
        delay(200);
        digitalWrite(USB_POWER, HIGH);
    }

    if (RFID.available()) // RFID 받으면
    {
        digitalWrite(USB_POWER, LOW);
        String RFID_STRING = rfidparse();
        PC.listen();
        datasend(RFID_STRING, "0", "0");
        delay(1000);
        unsigned long WAIT_PRV_MILLIS = millis();
        while (millis() < WAIT_PRV_MILLIS + (INPUT_TIMEOUT * 1000) - 1000) // WAITTIME동안 기다리기
        {
            if (scale.get_units(10) > 0.1) // 100그람 이상 무게 있으면
            {
                if (digitalRead(SCALE_MODE) == HIGH) //-------------------------------------------고속모드(일정 데이터 읽어서 평균)
                {
                FAIL:
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
                    AVG_DATA = errchk(RFID_STRING,AVG_DATA);
                    String FINAL_STRING = String(AVG_DATA, 3);
                    datasend(RFID_STRING, FINAL_STRING, "0");
                    motor(3100);                  // 모터 가동
                    digitalWrite(RST_HWPIN, LOW); //종료
                }
                //-------------------------------------------일반모드(안정화)
                if (digitalRead(SCALE_MODE) == LOW)
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
                            double AVG_DATA_FINAL = AVG_DATA / (TOLERANCE_ALLOWABLE_DATA * 2);
                            AVG_DATA_FINAL = errchk(RFID_STRING,AVG_DATA_FINAL);
                            String AVG_DATA_STR = String(AVG_DATA_FINAL, 3);
                            datasend(RFID_STRING, AVG_DATA_STR, "0");
                            motor(3100);                  // 모터 가동
                            digitalWrite(RST_HWPIN, LOW);
                        }
                    }
                    goto FAIL;
                }
            }
        }
        datasend(RFID_STRING, "0.000", "1");
        digitalWrite(RST_HWPIN, LOW); //종료
    }
}
