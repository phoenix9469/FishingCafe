#define I2C_LCD_ADDR 0x27
#define I2C_LCD_ROW 16
#define I2C_LCD_COL 2
#define DEBUG_BAUD 115200
#define SCALE_LOW_LIMIT 90     // 90g
#define SCALE_HIGH_LIMIT 10000 // 10000g = 10kg
#define MOTOR_HIGH_S 4000
#define MOTOR_LOW_S 4000

// 32G SD 31902400512
// uint64_t SD_MARGIN = 31902300512; // 32G SD for test
uint64_t SD_MARGIN = 1073741824; // 1GiB

// const char *http_username = "scale_admin";
// const char *http_password = "scale_admin";

struct SystemConfig
{
  String Device_Name = "WiFiScale_";
  String WiFi_SSID;
  String WiFi_PASS;
  String WiFi_IP;
  String WiFi_Gateway;
  String TCP_IP;
  String TCP_PORT;
  String RS232_BAUD;
  String COMM_MODE;
  String build_date = __DATE__ " " __TIME__;
  String today_date;
  String http_username;
  String http_password;
  bool mute = false;
  const char *ntpServer = "kr.pool.ntp.org";
  uint8_t timeZone = 9;
};

SystemConfig sysconfig;

struct ScaleConfig
{
  unsigned char INPUT_TIMEOUT;                 // 투입 타임아웃,초
  unsigned char AVERAGE_MODE_DATA_AMOUNT = 70; // 평균모드 데이터 개수,시간 계산법:70/10(Sample Per Second) = 7초
  unsigned char STBL_MODE_DATA_AMOUNT = 30;    // 안정화모드 데이터 개수,1Cycle당
  unsigned char STBL_MODE_TIMEOUT = 30;        // 안정화모드 타임아웃,초
  double STBL_TOLERANCE = 300.0;               // 안정화모드 무게 차이 허용치,g
  unsigned char TOLERANCE_ALLOWABLE_VAL = 10;  // 안정화 되었다고 판단하는 허용치 내 데이터 개수
  double NET_WEIGHT;                           // 뜰채 무게,g
  double CAL_WEIGHT;                           // 캘리브레이션 시 투입 무게(g)
  double FACTOR;                               // g 단위 변환용
};
ScaleConfig scaleconfig;