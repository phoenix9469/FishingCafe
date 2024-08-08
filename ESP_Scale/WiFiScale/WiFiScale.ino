#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "time.h"
#include "FS.h"
#include "SD.h"
#include "SPI.h"
#include <Wire.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <Preferences.h>
#include <nvs_flash.h>

#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LiquidCrystal_I2C.h>
#include <PCF8563.h>
#include <Adafruit_MCP23X17.h>
#include <Adafruit_NAU7802.h>
#include <ArduinoJson.h>
#include <ItemSubMenu.h>
#include <ItemCommand.h>
#include <LcdMenu.h>
#include <utils/commandProccesors.h>
#include <ESP32Ping.h>

#include "mcp_button.h"
#include "Asyncserver.h"

#define ARDUINOJSON_ENABLE_ARDUINO_STRING 1

// Pin Define
#define LED1 15 // STATUS
#define LED2 14 // WIFI
#define MOTOR1 25
#define MOTOR2 26
#define USER_232_RX 32
#define USER_232_TX 27
#define PANEL_INTERRUPT 34
#define SD_PWR 33
#define SD_DET 35

AsyncWebServer server(80);
WiFiClient client;
DynamicJsonDocument config_json(2048);
PCF8563 pcf;
LiquidCrystal_I2C lcd(I2C_LCD_ADDR, I2C_LCD_ROW, I2C_LCD_COL);
LcdMenu menu(I2C_LCD_COL, I2C_LCD_ROW);
Adafruit_MCP23X17 mcp;
Adafruit_NAU7802 nau;
Time nowTime;
Preferences preferences;

QueueHandle_t ButtonTaskQueue;
QueueHandle_t BuzzerTaskQueue;
QueueHandle_t StatusLEDTaskQueue;
QueueHandle_t MenuTaskQueue;
QueueHandle_t PingTaskQueue;

TaskHandle_t WifiLEDTaskHandle = NULL;
TaskHandle_t MenuTaskHandle = NULL;
TaskHandle_t PingTaskHandle = NULL;

hw_timer_t *timer_0 = NULL;
portMUX_TYPE timerMux = portMUX_INITIALIZER_UNLOCKED;

bool ButtonBlock = true;

bool RFIDeventFlag = false;
bool MeasureFlag = false;

bool NET_BTN_STATUS = true;
bool TARE_BTN_STATUS = true;
bool USER1_BTN_STATUS = true;
bool USER2_BTN_STATUS = true;
bool USER3_BTN_STATUS = true;
bool USER4_BTN_STATUS = true;

bool NET_BTN_CNT = 0;
bool TARE_BTN_CNT = 0;
bool USER1_BTN_CNT = 0;
bool USER2_BTN_CNT = 0;
bool USER3_BTN_CNT = 0;
bool USER4_BTN_CNT = 0;

unsigned char MENU_MODE_CNT = 0;
bool MENU_MODE = false;

unsigned long Millis = millis();
unsigned long previousMillis = 0;

bool rebooting = false;
bool wifi_enable = false;
bool wifi_retry = true;
bool SD_Status = 0;
unsigned char Ping_Fail_Cnt = 0;

bool Show_Loadcell_Value = false;

unsigned char rfid_buffer[13] = {0x00};

unsigned char DEBUG_MODE = 0;

void menu_scale_callibrate()
{
  config_json["factor"] = scale_callibrate();
  writeConfig();
}

void menu_exit()
{
  delay(1500);
  appendLogFile(true, "[SYSTEM] Exit Menu...", true);
  ESP.restart();
}

String Menu_WiFiConnected = "AP Mode";
String Menu_WiFiSSID = "-";
String Menu_MyIP = "Not Connected";
String Menu_TCPIP = "-";
String Menu_TCPPORT = "-";
String Menu_Comm_Mode = "MODE:";
String Menu_Baud = "BAUD:";
String Menu_Date = "DATE:";
String Menu_Input_Timeout = "Timeout:";
String Menu_Net_Weight = "Net:";
String Menu_Cal_Weight = "Cal Wt:";
String Menu_Factor = "Factor:";

extern MenuItem *NetworkInfo[];
extern MenuItem *TcpInfo[];
extern MenuItem *ScaleInfo[];
extern MenuItem *DeviceInfo[];

MAIN_MENU(
    ITEM_SUBMENU("1.Network Info", NetworkInfo),
    ITEM_SUBMENU("2.TCP/IP Info", TcpInfo),
    ITEM_SUBMENU("3.Scale Info", ScaleInfo),
    ITEM_SUBMENU("4.Device Info", DeviceInfo),
    ITEM_COMMAND("5.Callibrate", menu_scale_callibrate),
    ITEM_COMMAND("6.Exit&Reboot", menu_exit));
SUB_MENU(NetworkInfo, mainMenu,
         ITEM_BASIC(Menu_WiFiConnected.c_str()),
         ITEM_BASIC(Menu_WiFiSSID.c_str()),
         //  ITEM_BASIC("-"),
         ITEM_BASIC(Menu_MyIP.c_str()));
SUB_MENU(TcpInfo, mainMenu,
         ITEM_BASIC(Menu_TCPIP.c_str()),
         ITEM_BASIC(Menu_TCPPORT.c_str()));
SUB_MENU(ScaleInfo, mainMenu,
         ITEM_BASIC(Menu_Input_Timeout.c_str()),
         ITEM_BASIC(Menu_Net_Weight.c_str()),
         ITEM_BASIC(Menu_Cal_Weight.c_str()),
         ITEM_BASIC(Menu_Factor.c_str()));
SUB_MENU(DeviceInfo, mainMenu,
         ITEM_BASIC(sysconfig.Device_Name.c_str()),
         ITEM_BASIC(Menu_Comm_Mode.c_str()),
         ITEM_BASIC(Menu_Baud.c_str()),
         ITEM_BASIC(Menu_Date.c_str()));

const char *WiFiStatusCode(wl_status_t status)
{
  switch (status)
  {
  case WL_NO_SHIELD:
    return "WL_NO_SHIELD";
  case WL_IDLE_STATUS:
    return "WL_IDLE_STATUS";
  case WL_NO_SSID_AVAIL:
    return "WL_NO_SSID_AVAIL";
  case WL_SCAN_COMPLETED:
    return "WL_SCAN_COMPLETED";
  case WL_CONNECTED:
    return "WL_CONNECTED";
  case WL_CONNECT_FAILED:
    return "WL_CONNECT_FAILED";
  case WL_CONNECTION_LOST:
    return "WL_CONNECTION_LOST";
  case WL_DISCONNECTED:
    return "WL_DISCONNECTED";
  }
}

String reset_reason(int reason)
{
  switch (reason)
  {
  case 1:
    return "POWERON_RESET"; /**<1,  Vbat power on reset*/
  case 3:
    return "SW_RESET"; /**<3,  Software reset digital core*/
  case 4:
    return "OWDT_RESET"; /**<4,  Legacy watch dog reset digital core*/
  case 5:
    return "DEEPSLEEP_RESET"; /**<5,  Deep Sleep reset digital core*/
  case 6:
    return "SDIO_RESET"; /**<6,  Reset by SLC module, reset digital core*/
  case 7:
    return "TG0WDT_SYS_RESET"; /**<7,  Timer Group0 Watch dog reset digital core*/
  case 8:
    return "TG1WDT_SYS_RESET"; /**<8,  Timer Group1 Watch dog reset digital core*/
  case 9:
    return "RTCWDT_SYS_RESET"; /**<9,  RTC Watch dog Reset digital core*/
  case 10:
    return "INTRUSION_RESET"; /**<10, Instrusion tested to reset CPU*/
  case 11:
    return "TGWDT_CPU_RESET"; /**<11, Time Group reset CPU*/
  case 12:
    return "SW_CPU_RESET"; /**<12, Software reset CPU*/
  case 13:
    return "RTCWDT_CPU_RESET"; /**<13, RTC Watch dog Reset CPU*/
  case 14:
    return "EXT_CPU_RESET"; /**<14, for APP CPU, reseted by PRO CPU*/
  case 15:
    return "RTCWDT_BROWN_OUT_RESET"; /**<15, Reset when the vdd voltage is not stable*/
  case 16:
    return "RTCWDT_RTC_RESET"; /**<16, RTC Watch dog reset digital core and rtc module*/
  default:
    return "NO_MEAN";
  }
}

String readFile(fs::FS &fs, const char *path)
{
  // Serial.printf("Reading file: %s\n", path);
  String data = "";
  File file = fs.open(path);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return data;
  }
  while (file.available())
  {
    char ch1 = file.read();
    data = String(data + String(ch1));
  }
  file.close();
  return data;
}

void writeFile(fs::FS &fs, const char *path, const char *message)
{
  // Serial.printf("Writing file: %s\n", path);
  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    // Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}

void deleteFile(fs::FS &fs, const char *path)
{
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path))
  {
    Serial.println("File deleted");
  }
  else
  {
    Serial.println("Delete failed");
  }
}

void appendFile(fs::FS &fs, const char *path, const char *message)
{
  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message))
  {
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}

void appendLogFile(bool timecode, String message, bool crlf)
{
  nowTime = pcf.getTime();
  sysconfig.today_date = String(nowTime.year) + "_" + String(nowTime.month) + "_" + String(nowTime.day);
  String log_path = "/log/" + sysconfig.today_date + ".txt";
  String append_msg;
  if (timecode)
    append_msg = "[" + String(nowTime.year) + "/" + String(nowTime.month) + "/" + String(nowTime.day) + " " + String(nowTime.hour) + ":" + String(nowTime.minute) + ":" + String(nowTime.second) + "]";
  append_msg = append_msg + message;
  if (crlf)
    append_msg = append_msg + "\r\n";
  if (DEBUG_MODE == 1)
  {
    Serial.print(append_msg);
  }
  else if (DEBUG_MODE == 2)
  {
    Serial1.print(append_msg);
  }

  appendFile(SD, log_path.c_str(), append_msg.c_str());
}

void writeConfig()
{
  String output;
  serializeJsonPretty(config_json, output);
  writeFile(SD, "/config/config.txt", output.c_str());
  return;
}

uint64_t getTotalUsedBytes(const char *path)
{
  File dir = SD.open(path);
  uint64_t totalSize = 0;
  while (true)
  {
    File entry = dir.openNextFile();
    if (!entry)
    {
      break;
    }
    if (!entry.isDirectory())
    {
      totalSize += entry.size();
    }
    entry.close();
  }
  return totalSize;
}

int initSDCard()
{
  if (digitalRead(SD_DET))
  {
    Serial.println("No SD card attached");
    return 1;
  }

  if (!SD.begin())
  {
    Serial.println("Card Mount Failed");
    return 1;
  }

  uint8_t cardType = SD.cardType();

  if (cardType == CARD_NONE)
  {
    Serial.println("No SD card attached");
    return 1;
  }

  uint64_t total_memory = SD.totalBytes();
  uint64_t used_memory = getTotalUsedBytes("/log");
  uint64_t left_memory = total_memory - used_memory;
  if (left_memory <= SD_MARGIN) // Less Then 1Gib
  {
    while (left_memory <= SD_MARGIN)
    {
      // Delete Some log...
      File log = SD.open("/log");
      File file = log.openNextFile();
      time_t old_file = 2147483647;
      String oldest_file_name;
      while (file)
      {
        if (old_file > file.getLastWrite())
        {
          old_file = file.getLastWrite();
          oldest_file_name = String(file.name());
        }
        file = log.openNextFile();
      }
      String old_file_path = "/log/" + oldest_file_name;
      deleteFile(SD, old_file_path.c_str());
      used_memory = getTotalUsedBytes("/log");
      left_memory = total_memory - used_memory;
    }
  }
  return 0;
}

void SendData(bool is_log_message, String log_message, unsigned char data_to_send[], unsigned int data_length)
{
  if (is_log_message)
  {
    appendLogFile(true, log_message, true);
  }
  String data_str(data_to_send, data_length);
  appendLogFile(true, data_str, true);
  Serial1.write(data_to_send, data_length);
  if (wifi_enable)
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      appendLogFile(true, "TCP/IP Send Fail-Wifi Not Connected", true);
    }
    else if (client.connected() != true)
    {
      appendLogFile(true, "TCP/IP Send Fail-TCP/IP Server Not Connected", true);
    }
    else
    {
      client.write(data_to_send, data_length);
    }
  }
}

int ntp_sync()
{
  configTime(sysconfig.timeZone * 3600, 0, sysconfig.ntpServer);
  struct tm timeinfo;
  if (getLocalTime(&timeinfo))
  {
    char timeYear[3];
    char timeMonth[3];
    char timeDay[3];
    char timeHour[3];
    char timeMinute[3];
    char timeSecond[3];
    strftime(timeYear, 3, "%g", &timeinfo);
    strftime(timeMonth, 3, "%m", &timeinfo);
    strftime(timeDay, 3, "%d", &timeinfo);
    strftime(timeHour, 3, "%H", &timeinfo);
    strftime(timeMinute, 3, "%M", &timeinfo);
    strftime(timeSecond, 3, "%S", &timeinfo);
    int Tyear = atoi(timeYear);
    int Tmonth = atoi(timeMonth);
    int Tday = atoi(timeDay);
    int Thour = atoi(timeHour);
    int Tminute = atoi(timeMinute);
    int Tsecond = atoi(timeSecond);
    pcf.stopClock();
    pcf.setYear(Tyear);
    pcf.setMonth(Tmonth);
    pcf.setDay(Tday);
    pcf.setHour(Thour);
    pcf.setMinut(Tminute);
    pcf.setSecond(Tsecond);
    pcf.startClock();
    appendLogFile(true, "NTP Sync Success", true);
  }
  else
  {
    appendLogFile(true, "NTP Server Fail", true);
    return 1;
  }
  return 0;
}

void set_internal_time()
{
  nowTime = pcf.getTime();
  struct tm tm_in;
  tm_in.tm_year = (2000 + nowTime.year) - 1900;
  tm_in.tm_mon = nowTime.month - 1;
  tm_in.tm_mday = nowTime.day;
  tm_in.tm_hour = nowTime.hour;
  tm_in.tm_min = nowTime.minute;
  tm_in.tm_sec = nowTime.second;
  time_t ts = mktime(&tm_in) - sysconfig.timeZone * 3600; // RTC-Local Time, ESP32 System Time-UTC
  // printf("Setting time: %s", asctime(&tm_in));
  struct timeval now = {.tv_sec = ts};
  settimeofday(&now, NULL);
}

int setup_wifi(int wifi_interval)
{
  vTaskResume(WifiLEDTaskHandle);
  if (sysconfig.WiFi_SSID == "")
  {
    Serial.println("No SSID");
    return false;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setHostname(sysconfig.Device_Name.c_str());
  WiFi.begin(sysconfig.WiFi_SSID, sysconfig.WiFi_PASS);
  appendLogFile(true, "Connecting to WiFi ", false);
  appendLogFile(false, sysconfig.WiFi_SSID, true);
  lcd.setCursor(0, 0);
  lcd.print("WIFI CONNECTING");

  unsigned long currentMillis = millis();
  previousMillis = currentMillis;

  // wifi_interval동안 와이파이 연결 시도
  while (WiFi.status() != WL_CONNECTED)
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= wifi_interval)
    {
      appendLogFile(true, "Failed to Connect Wifi", false);
      lcd.setCursor(0, 1);
      lcd.print("WIFI Failed");
      return 1;
    }
  }

  ntp_sync();

  String IP = WiFi.localIP().toString();
  Menu_MyIP = IP;
  Menu_WiFiConnected = "Connected";
  appendLogFile(true, "WiFi Connected. IP : ", false);
  appendLogFile(false, IP.c_str(), true);
  MDNS.begin(sysconfig.Device_Name.c_str());
  setupAsyncServer();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WIFI CONNECTED");
  lcd.setCursor(0, 1);
  lcd.print(IP.c_str());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("TCP/IP SERVER");
  lcd.setCursor(0, 1);
  lcd.print("CONNECTING...");
  currentMillis = millis();
  previousMillis = currentMillis;

  // wifi_interval동안 TCP/IP 서버 연결 시도
  while (!client.connect(sysconfig.TCP_IP.c_str(), sysconfig.TCP_PORT.toInt()))
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= wifi_interval)
    {
      appendLogFile(true, "Failed to Connect TCP/IP Server", false);
      lcd.setCursor(0, 1);
      lcd.print("TCP/IP Failed");
      return 1;
    }
  }
  appendLogFile(true, "TCP/IP Server Connected. IP : ", false);
  appendLogFile(false, sysconfig.TCP_IP.c_str(), true);
  vTaskResume(PingTaskHandle);
  vTaskSuspend(WifiLEDTaskHandle);
  digitalWrite(LED2, HIGH);
  scale_init();
  return 0;
}

double read_kg()
{
  if (nau.available())
  {
    int32_t val = nau.read();
    double kg_val = (double)val / scaleconfig.FACTOR;
    return kg_val;
  }
  else
  {
    return -255.0;
  }
}

int scale_init()
{
  RFIDeventFlag = 0;
  MENU_MODE_CNT = 0;

  NET_BTN_STATUS = 1;
  TARE_BTN_STATUS = 1;
  USER1_BTN_STATUS = 1;
  USER2_BTN_STATUS = 1;
  USER3_BTN_STATUS = 1;
  USER4_BTN_STATUS = 1;
  mcp.digitalWrite(NET_LED, NET_BTN_STATUS);
  mcp.digitalWrite(TARE_LED, TARE_BTN_STATUS);
  mcp.digitalWrite(USER1_LED, USER1_BTN_STATUS);
  mcp.digitalWrite(USER2_LED, USER2_BTN_STATUS);
  mcp.digitalWrite(USER3_LED, USER3_BTN_STATUS);
  mcp.digitalWrite(USER4_LED, USER4_BTN_STATUS);

  nau.calibrate(NAU7802_CALMOD_INTERNAL);
  nau.calibrate(NAU7802_CALMOD_OFFSET);
  for (uint8_t i = 0; i < 10; i++)
  {
    if (nau.available())
    {
      nau.read();
    }
    else
    {
      i--;
    }
  }
  unsigned char pc_init_message[13] = {'R', 'E', 'A', 'D', 'Y', '|', '0', '|', '0', '|', '0', 0x0D, 0x0A};
  SendData(true, "Init..", pc_init_message, sizeof(pc_init_message));
  bool BuzzBuzzer = true;
  xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
  bool BlinkStatusLED = true;
  xQueueSend(StatusLEDTaskQueue, &BlinkStatusLED, portMAX_DELAY);
  Serial2.begin(9600);
  mcp.digitalWrite(LED_STRIP, HIGH);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("READY!!!");
  ButtonBlock = false;
  Show_Loadcell_Value = true;
  return 0;
}

double scale_callibrate()
{
  double var = 0;
  double scale = 0;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CALLIBRATE MODE");
  lcd.setCursor(0, 1);
  lcd.print("DO NOT TOUCH!");
  vTaskDelay(pdMS_TO_TICKS(2500));
  nau.calibrate(NAU7802_CALMOD_INTERNAL);
  nau.calibrate(NAU7802_CALMOD_OFFSET);
  vTaskDelay(pdMS_TO_TICKS(100));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("PLACE A WEIGHT");
  lcd.setCursor(0, 1);
  lcd.print(scaleconfig.CAL_WEIGHT);
  lcd.print("g");
  vTaskDelay(pdMS_TO_TICKS(2500));
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WAIT...");
  for (int i = 0; i < 10; i++)
  {
    if (nau.available())
    {
      int32_t val = nau.read();
      var = var + val;
    }
    else
    {
      i--;
    }
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  var = var / 10;
  scale = var / scaleconfig.CAL_WEIGHT; // 센서값 / 아는값
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CALLIBRATE END");
  lcd.setCursor(0, 1);
  lcd.print(scale);
  vTaskDelay(pdMS_TO_TICKS(1500));
  return scale;
}

void scale_tare()
{
  mcp.digitalWrite(LED_STRIP, LOW);
  vTaskDelay(pdMS_TO_TICKS(1000));
  nau.calibrate(NAU7802_CALMOD_INTERNAL);
  nau.calibrate(NAU7802_CALMOD_OFFSET);
  mcp.digitalWrite(LED_STRIP, HIGH);
  return;
}

double averageMode()
{
  double READ_DATA[scaleconfig.AVERAGE_MODE_DATA_AMOUNT];
  double AVG_DATA = 0;
  for (int i = 0; i < scaleconfig.AVERAGE_MODE_DATA_AMOUNT; i++)
  {
    if (nau.available())
    {
      int32_t val = nau.read();
      double RAW_DATA = (double)val / scaleconfig.FACTOR;
      if (RAW_DATA > SCALE_LOW_LIMIT && RAW_DATA < SCALE_HIGH_LIMIT)
      {
        READ_DATA[i] = RAW_DATA;
        // Serial.println(RAW_DATA);
      }
      else
      {
        i--;
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
  for (int i = 0; i < scaleconfig.AVERAGE_MODE_DATA_AMOUNT; i++)
  {
    AVG_DATA = AVG_DATA + READ_DATA[i];
    // Serial.println(AVG_DATA);
  }
  AVG_DATA = AVG_DATA / scaleconfig.AVERAGE_MODE_DATA_AMOUNT;
  return AVG_DATA;
}

double stableMode()
{
  unsigned long NORM_PRV_MILLIS = millis();
  while (millis() < NORM_PRV_MILLIS + scaleconfig.STBL_MODE_TIMEOUT * 1000)
  {
    double READ_DATA[scaleconfig.STBL_MODE_DATA_AMOUNT] = {0};
    double DIFF_ARR[scaleconfig.STBL_MODE_DATA_AMOUNT - 1] = {0};
    double AVG_DATA = 0;
    int TOLERANCE_ALLOWABLE_DATA = 0;
    bool ExceptionFlag = false;
    for (int i = 0; i < scaleconfig.STBL_MODE_DATA_AMOUNT; i++)
    {
      if (nau.available())
      {
        int32_t val = nau.read();
        double RAW_DATA = (double)val / scaleconfig.FACTOR;
        if (RAW_DATA > SCALE_LOW_LIMIT && RAW_DATA < SCALE_HIGH_LIMIT)
        {
          READ_DATA[i] = RAW_DATA;
          // Serial.println(RAW_DATA);
        }
        else
        {
          i = i - 2;
          if (i <= 0)
          {
            ExceptionFlag = true;
            break;
          }
        }
      }
      else
      {
        i--;
      }
    }
    if (!ExceptionFlag)
    {
      for (int i = 0; i < scaleconfig.STBL_MODE_DATA_AMOUNT - 1; i++)
      {
        double temp = READ_DATA[i] - READ_DATA[i + 1];
        DIFF_ARR[i] = abs(temp);
        // Serial.print("Absolute:");
        // Serial.println(DIFF_ARR[i]);
      }
      for (int i = 0; i < scaleconfig.STBL_MODE_DATA_AMOUNT - 1; i++)
      {
        if (DIFF_ARR[i] < scaleconfig.STBL_TOLERANCE)
        {
          AVG_DATA = AVG_DATA + READ_DATA[i] + READ_DATA[i + 1];
          TOLERANCE_ALLOWABLE_DATA++;
          // Serial.print("Tolerance:");
          // Serial.println(TOLERANCE_ALLOWABLE_DATA);
        }
      }
      // Serial.print("Data Amount:");
      // Serial.println(TOLERANCE_ALLOWABLE_DATA);
      if (TOLERANCE_ALLOWABLE_DATA >= scaleconfig.TOLERANCE_ALLOWABLE_VAL)
      {
        double AVG_DATA_FINAL = AVG_DATA / (TOLERANCE_ALLOWABLE_DATA * 2);
        // Serial.println(AVG_DATA_FINAL);
        return AVG_DATA_FINAL;
        break;
      }
    }
  }
  return -1;
}

unsigned char i2c_device_chk()
{
  unsigned char i2c_error = 0;
  Wire.beginTransmission(0x20); // MCP23017
  i2c_error = i2c_error + Wire.endTransmission();

  Wire.beginTransmission(I2C_LCD_ADDR); // I2C LCD
  i2c_error = i2c_error + Wire.endTransmission();

  Wire.beginTransmission(0x2A); // NAU7802
  i2c_error = i2c_error + Wire.endTransmission();

  Wire.beginTransmission(0x51); // PCF8563
  i2c_error = i2c_error + Wire.endTransmission();
  return i2c_error;
}

// Interrupt Service Routine
void IRAM_ATTR PanelISR()
{
  bool btn_pressed = true;
  BaseType_t xHigherPriorityTaskWoken = pdFALSE;
  xQueueSendFromISR(ButtonTaskQueue, &btn_pressed, &xHigherPriorityTaskWoken);

  if (xHigherPriorityTaskWoken)
  {
    portYIELD_FROM_ISR();
  }
}

void IRAM_ATTR PanelTimer() // Panel Detatched?
{
  portENTER_CRITICAL_ISR(&timerMux);
  digitalWrite(LED1, HIGH);
  rebooting = true;
  portEXIT_CRITICAL_ISR(&timerMux);
}

void IRAM_ATTR SDISR()
{
  SD_Status = digitalRead(SD_DET);
}

// FreeRTOS Tasks
void ButtonTask(void *pvParameters)
{
  bool btn_pressed = false;
  while (true)
  {
    if (xQueueReceive(ButtonTaskQueue, &btn_pressed, portMAX_DELAY))
    {
      if (digitalRead(PANEL_INTERRUPT) == LOW)
      {
        timerWrite(timer_0, 0);
        timerAlarmEnable(timer_0);
        unsigned char int_pin = mcp.getLastInterruptPin();
        mcp.clearInterrupts();
        button(int_pin);
      }
      else if (digitalRead(PANEL_INTERRUPT) == HIGH)
      {
        timerAlarmDisable(timer_0);
        timerWrite(timer_0, 0);
      }
    }
  }
}

void BuzzerTask(void *pvParameters)
{
  bool BuzzBuzzer = false;
  while (true)
  {
    if (xQueueReceive(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY))
    {
      if (!sysconfig.mute)
      {
        mcp.digitalWrite(BUZZER, HIGH);
        vTaskDelay(pdMS_TO_TICKS(100));
        mcp.digitalWrite(BUZZER, LOW);
      }
    }
  }
}

void PingTask(void *pvParameters)
{
  while (true)
  {
    if (wifi_enable)
    {
      if (Ping.ping(sysconfig.TCP_IP.c_str(), 1))
      {
        // Serial.println("Ping Success");
        Ping_Fail_Cnt = 0;
      }
      else
      {
        // Serial.println("Ping Failed");
        Ping_Fail_Cnt++;
        if (Ping_Fail_Cnt > 1)
        {
          appendLogFile(true, "Ping Failed", true);
          client.stop();
          vTaskSuspend(PingTaskHandle);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
}

void StatusLEDTask(void *pvParameters)
{
  bool BlinkStatusLED = false;
  while (true)
  {
    if (xQueueReceive(StatusLEDTaskQueue, &BlinkStatusLED, portMAX_DELAY))
    {
      digitalWrite(LED1, HIGH);
      vTaskDelay(pdMS_TO_TICKS(50));
      digitalWrite(LED1, LOW);
    }
  }
}

void WifiLEDTask(void *pvParameters)
{
  while (true)
  {
    digitalWrite(LED2, HIGH);
    vTaskDelay(pdMS_TO_TICKS(100));
    digitalWrite(LED2, LOW);
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void MenuTask(void *pvParameters)
{
  unsigned char ButtonValue = 0;
  while (true)
  {
    if (xQueueReceive(MenuTaskQueue, &ButtonValue, portMAX_DELAY))
    {
      switch (ButtonValue)
      {
      case CONFIG_START:
        appendLogFile(true, "[SYSTEM] Entering Menu...", true);
        menu.setupLcdWithMenu(I2C_LCD_ADDR, mainMenu);
        break;
      case CONFIG_UP:
        menu.up();
        break;
      case CONFIG_DOWN:
        menu.down();
        break;
      case CONFIG_BACK:
        menu.back();
        break;
      case CONFIG_ENTER:
        menu.enter();
        break;
      }
    }
  }
}

void setup()
{
  delay(100);
  esp_reset_reason_t rst_reason = esp_reset_reason();
  Serial.begin(DEBUG_BAUD);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(MOTOR1, OUTPUT);
  pinMode(MOTOR2, OUTPUT);
  pinMode(SD_PWR, OUTPUT);
  digitalWrite(LED1, LOW);
  digitalWrite(LED2, LOW);
  digitalWrite(MOTOR1, LOW);
  digitalWrite(MOTOR2, LOW);
  pinMode(SD_DET, INPUT);

  // Queue 생성
  ButtonTaskQueue = xQueueCreate(10, sizeof(int));
  BuzzerTaskQueue = xQueueCreate(10, sizeof(int));
  StatusLEDTaskQueue = xQueueCreate(10, sizeof(int));
  MenuTaskQueue = xQueueCreate(10, sizeof(int));
  PingTaskQueue = xQueueCreate(10, sizeof(int));

  // Task 생성
  xTaskCreate(ButtonTask, "ButtonTask", 2048, NULL, 3, NULL);
  xTaskCreate(BuzzerTask, "BuzzerTask", 2048, NULL, 2, NULL);
  xTaskCreate(PingTask, "PingTask", 3072, NULL, 2, &PingTaskHandle);
  xTaskCreate(StatusLEDTask, "StatusLEDTask", 2048, NULL, 1, NULL);
  xTaskCreate(WifiLEDTask, "WifiLEDTask", 2048, NULL, 1, &WifiLEDTaskHandle);
  xTaskCreate(MenuTask, "MenuTask", 4096, NULL, 1, &MenuTaskHandle);
  vTaskSuspend(MenuTaskHandle);
  vTaskSuspend(PingTaskHandle);

  timer_0 = timerBegin(0, 80, true);
  timerAttachInterrupt(timer_0, &PanelTimer, true);
  timerAlarmWrite(timer_0, 2000000, true);
  attachInterrupt(digitalPinToInterrupt(PANEL_INTERRUPT), PanelISR, CHANGE);
  attachInterrupt(digitalPinToInterrupt(SD_DET), SDISR, CHANGE);

  Wire.begin();

  if (i2c_device_chk() != 0)
  {
    for (int i = 0; i < 3; i++)
    {
      bool BlinkStatusLED = true;
      xQueueSend(StatusLEDTaskQueue, &BlinkStatusLED, portMAX_DELAY);
      delay(300);
    }
    ESP.restart();
  }

  pcf.init();
  nowTime = pcf.getTime();
  sysconfig.today_date = String(nowTime.year) + "_" + String(nowTime.month) + "_" + String(nowTime.day);
  set_internal_time();

  if (!mcp.begin_I2C())
  {
    Serial.println("MCP Init FAIL");
  }
  mcp.pinMode(LED_STRIP, OUTPUT);
  mcp.pinMode(BUZZER, OUTPUT);
  mcp.pinMode(NET_LED, OUTPUT);
  mcp.pinMode(TARE_LED, OUTPUT);
  mcp.pinMode(USER1_LED, OUTPUT);
  mcp.pinMode(USER2_LED, OUTPUT);
  mcp.pinMode(USER3_LED, OUTPUT);
  mcp.pinMode(USER4_LED, OUTPUT);
  mcp.pinMode(NET_BTN, INPUT);
  mcp.pinMode(TARE_BTN, INPUT);
  mcp.pinMode(USER1_BTN, INPUT);
  mcp.pinMode(USER2_BTN, INPUT);
  mcp.pinMode(USER3_BTN, INPUT);
  mcp.pinMode(USER4_BTN, INPUT);

  mcp.digitalWrite(LED_STRIP, LOW);
  mcp.digitalWrite(BUZZER, LOW);
  mcp.digitalWrite(NET_LED, NET_BTN_STATUS);
  mcp.digitalWrite(TARE_LED, TARE_BTN_STATUS);
  mcp.digitalWrite(USER1_LED, USER1_BTN_STATUS);
  mcp.digitalWrite(USER2_LED, USER2_BTN_STATUS);
  mcp.digitalWrite(USER3_LED, USER3_BTN_STATUS);
  mcp.digitalWrite(USER4_LED, USER4_BTN_STATUS);
  mcp.setupInterrupts(true, false, LOW);
  mcp.setupInterruptPin(NET_BTN, CHANGE);
  mcp.setupInterruptPin(TARE_BTN, CHANGE);
  mcp.setupInterruptPin(USER1_BTN, CHANGE);
  mcp.setupInterruptPin(USER2_BTN, CHANGE);
  mcp.setupInterruptPin(USER3_BTN, CHANGE);
  mcp.setupInterruptPin(USER4_BTN, CHANGE);
  mcp.clearInterrupts();

  if (!nau.begin())
  {
    Serial.println("Loadcell ADC Init FAIL");
    ESP.restart();
  }
  nau.setLDO(NAU7802_3V0);
  nau.setGain(NAU7802_GAIN_64);
  nau.setRate(NAU7802_RATE_10SPS);
  for (uint8_t i = 0; i < 10; i++)
  {
    while (!nau.available())
      delay(1);
    nau.read();
  }

  preferences.begin("info", false);
  sysconfig.Device_Name = sysconfig.Device_Name + preferences.getString("SN", "000");
  sysconfig.http_username = preferences.getString("HTTP_USER", "12345678");
  sysconfig.http_password = preferences.getString("HTTP_PASS", "12345678");
  DEBUG_MODE = preferences.getChar("DEBUG_MODE", 0);

  lcd.init();
  lcd.clear();
  lcd.backlight();
  lcd.setCursor(0, 0);
  lcd.print("INIT...");
  lcd.setCursor(0, 1);
  lcd.print(sysconfig.today_date);

  SD_Status = digitalRead(SD_DET);
  if (initSDCard())
  {
    Serial.println("SD CARD ERR");
    lcd.setCursor(0, 1);
    lcd.print("SD CARD ERROR");
    for (int i = 0; i < 3; i++)
    {
      bool BuzzBuzzer = true;
      xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
      delay(300);
    }
    ESP.restart();
  }

  deserializeJson(config_json, readFile(SD, "/config/config.txt"));
  sysconfig.WiFi_SSID = config_json["ssid"].as<String>();
  sysconfig.WiFi_PASS = config_json["pass"].as<String>();
  sysconfig.TCP_IP = config_json["tcp_ip"].as<String>();
  sysconfig.TCP_PORT = config_json["tcp_port"].as<String>();
  sysconfig.RS232_BAUD = config_json["baud"].as<String>();
  sysconfig.COMM_MODE = config_json["mode"].as<String>();
  sysconfig.mute = config_json["mute"].as<bool>();
  scaleconfig.NET_WEIGHT = config_json["net_weight"].as<double>();
  scaleconfig.CAL_WEIGHT = config_json["cal_weight"].as<double>();
  scaleconfig.FACTOR = config_json["factor"].as<double>();
  scaleconfig.INPUT_TIMEOUT = config_json["timeout"].as<int>();

  Menu_WiFiSSID = sysconfig.WiFi_SSID;
  Menu_MyIP = "Not Connected";
  Menu_TCPIP = sysconfig.TCP_IP;
  Menu_TCPPORT = sysconfig.TCP_PORT;
  Menu_Comm_Mode = Menu_Comm_Mode + sysconfig.COMM_MODE;
  Menu_Baud = Menu_Baud + sysconfig.RS232_BAUD;
  Menu_Date = Menu_Date + sysconfig.today_date;
  Menu_Input_Timeout = Menu_Input_Timeout + String(scaleconfig.INPUT_TIMEOUT) + "Sec";
  Menu_Net_Weight = Menu_Net_Weight + String(scaleconfig.NET_WEIGHT, 1) + "g";
  Menu_Cal_Weight = Menu_Cal_Weight + String(scaleconfig.CAL_WEIGHT, 1) + "g";
  Menu_Factor = Menu_Factor + String(scaleconfig.FACTOR, 2);

  if (sysconfig.COMM_MODE == "RS232")
  {
    wifi_enable = false;
  }
  else if (sysconfig.COMM_MODE == "TCP/IP")
  {
    wifi_enable = true;
  }
  appendLogFile(true, "==LOADCELL TEST MODE===", true);
  appendLogFile(true, "===[SYSTEM] BOOT ===", true);
  appendLogFile(true, "[SYSTEM] RESET : ", false);
  appendLogFile(false, reset_reason(rst_reason), true);
  appendLogFile(true, "[SYSTEM] SN : ", false);
  appendLogFile(false, sysconfig.Device_Name, true);
  appendLogFile(true, "[SYSTEM] SSID : ", false);
  appendLogFile(false, sysconfig.WiFi_SSID, true);
  appendLogFile(true, "[SYSTEM] PASS : ", false);
  appendLogFile(false, sysconfig.WiFi_PASS, true);
  appendLogFile(true, "[SYSTEM] TCP_IP : ", false);
  appendLogFile(false, sysconfig.TCP_IP, true);
  appendLogFile(true, "[SYSTEM] TCP_PORT : ", false);
  appendLogFile(false, sysconfig.TCP_PORT, true);
  appendLogFile(true, "[SYSTEM] RS232_BAUD : ", false);
  appendLogFile(false, sysconfig.RS232_BAUD, true);
  appendLogFile(true, "[SYSTEM] COMM_MODE : ", false);
  appendLogFile(false, sysconfig.COMM_MODE, true);
  appendLogFile(true, "[SYSTEM] MUTE : ", false);
  appendLogFile(false, String(sysconfig.mute), true);
  appendLogFile(true, "[SYSTEM] NET_WEIGHT : ", false);
  appendLogFile(false, String(scaleconfig.NET_WEIGHT), true);
  appendLogFile(true, "[SYSTEM] CAL_WEIGHT : ", false);
  appendLogFile(false, String(scaleconfig.CAL_WEIGHT), true);
  appendLogFile(true, "[SYSTEM] FACTOR : ", false);
  appendLogFile(false, String(scaleconfig.FACTOR), true);
  appendLogFile(true, "[SYSTEM] TIMEOUT : ", false);
  appendLogFile(false, String(scaleconfig.INPUT_TIMEOUT), true);

  Serial1.begin(sysconfig.RS232_BAUD.toInt(), SERIAL_8N1, USER_232_RX, USER_232_TX);

  if (wifi_enable == false || setup_wifi(30000) == 1)
  {
    appendLogFile(true, "[SYSTEM] Enter AP Config Mode", true);
    WiFi.disconnect();
    WiFi.softAP(sysconfig.Device_Name.c_str(), sysconfig.http_password.c_str());
    IPAddress IP = WiFi.softAPIP();
    MDNS.begin(sysconfig.Device_Name.c_str());
    setupAsyncServer();
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("AP CONFIG MODE");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.softAPIP().toString());
    Menu_WiFiSSID = sysconfig.Device_Name;
    Menu_MyIP = WiFi.softAPIP().toString();
    wifi_retry = false;
    if (wifi_enable == false)
    {
      vTaskSuspend(WifiLEDTaskHandle);
      digitalWrite(LED2, LOW);
      scale_init();
    }
  }
}

void loop()
{

  if (Show_Loadcell_Value)
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(nau.read());
    vTaskDelay(pdMS_TO_TICKS(10));
    lcd.setCursor(0, 1);
    lcd.print(read_kg());
    vTaskDelay(pdMS_TO_TICKS(500));
  }

  Millis = millis();

  if (rebooting)
  {
    delay(100);
    ESP.restart();
  }

  if (WiFi.status() != WL_CONNECTED && wifi_enable && wifi_retry)
  {
    vTaskResume(WifiLEDTaskHandle);
    mcp.digitalWrite(LED_STRIP, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("WIFI LOST...");
    appendLogFile(true, "WiFi Lost. Retrying...", true);
    WiFi.disconnect(true);
    client.stop();
    if (setup_wifi(60000) == 1)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("AP CONFIG MODE");
      lcd.setCursor(0, 1);
      lcd.print(sysconfig.Device_Name);
      appendLogFile(true, "WiFi Reconect Fail. Giveup", true);
      WiFi.disconnect();
      WiFi.softAP(sysconfig.Device_Name.c_str(), sysconfig.http_password.c_str());
      IPAddress IP = WiFi.softAPIP();
      Menu_WiFiSSID = sysconfig.Device_Name;
      Menu_MyIP = WiFi.softAPIP().toString();
      wifi_retry = false;
    }
  }

  if (client.connected() != true && wifi_enable && wifi_retry)
  {
    vTaskResume(WifiLEDTaskHandle);
    mcp.digitalWrite(LED_STRIP, LOW);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("TCP/IP SERVER");
    lcd.setCursor(0, 1);
    lcd.print("Disconnected");
    appendLogFile(true, "TCP/IP Server Lost. Retrying...", true);
    unsigned long currentMillis = millis();
    previousMillis = currentMillis;
    while (client.connected() != true)
    {
      currentMillis = millis();
      if (currentMillis - previousMillis >= 1000)
      {
        previousMillis = currentMillis;
        client.stop();
        appendLogFile(true, "TCP/IP Failed to connect. Retry", true);
        if (client.connect(sysconfig.TCP_IP.c_str(), sysconfig.TCP_PORT.toInt()) == true)
        {
          appendLogFile(true, "TCP/IP Server Connected", true);
          vTaskResume(PingTaskHandle);
          vTaskSuspend(WifiLEDTaskHandle);
          digitalWrite(LED2, HIGH);
          scale_init();
          break;
        }
      }
    }
  }

  if (Serial1.available() && (MENU_MODE || !wifi_retry))
  {
    if (Serial1.read() == 0x02)
    {
      char SerialBuffer[100];
      String Buffer = Serial1.readStringUntil(0x03);
      Buffer.toCharArray(SerialBuffer, 100);
      if (SerialBuffer[1] == 0x10)
      {
        char SNBuffer[10];
        for (int i = 0; i <= SerialBuffer[0]; i++)
        {
          if (SerialBuffer[0] == i)
          {
            SNBuffer[i] = '\0';
          }
          else
          {
            SNBuffer[i] = SerialBuffer[i + 2];
          }
        }
        Serial1.print("SN Set to:");
        Serial1.println(SNBuffer);
        preferences.putString("SN", String(SNBuffer));
      }
      else if (SerialBuffer[1] == 0x11)
      {
        char UserBuffer[30];
        for (int i = 0; i <= SerialBuffer[0]; i++)
        {
          if (SerialBuffer[0] == i)
          {
            UserBuffer[i] = '\0';
          }
          else
          {
            UserBuffer[i] = SerialBuffer[i + 2];
          }
        }
        Serial1.print("HTTP Username Set to:");
        Serial1.println(UserBuffer);
        preferences.putString("HTTP_USER", String(UserBuffer));
      }
      else if (SerialBuffer[1] == 0x12)
      {
        if (SerialBuffer[0] >= 8)
        {
          char PasswordBuffer[30];
          for (int i = 0; i <= SerialBuffer[0]; i++)
          {
            if (SerialBuffer[0] == i)
            {
              PasswordBuffer[i] = '\0';
            }
            else
            {
              PasswordBuffer[i] = SerialBuffer[i + 2];
            }
          }
          Serial1.print("HTTP Password Set to:");
          Serial1.println(PasswordBuffer);
          preferences.putString("HTTP_PASS", String(PasswordBuffer));
        }
        else
        {
          Serial1.print("Password Must Be Longer Than 8 Character");
        }
      }
      else if (SerialBuffer[1] == 0x13)
      {
        Serial1.println("Erase NVS And Reboot");
        nvs_flash_erase();
        nvs_flash_init();
        delay(100);
        ESP.restart();
      }
      else if (SerialBuffer[1] == 0x14)
      {
        Serial1.println("Factory Reset");
        nvs_flash_erase();
        nvs_flash_init();
        delay(100);
        config_json["ssid"] = "12345678";
        config_json["pass"] = "12345678";
        config_json["tcp_ip"] = "192.168.0.10";
        config_json["tcp_port"] = "8888";
        config_json["baud"] = "300";
        config_json["mute"] = false;
        config_json["net_weight"] = 0.3;
        config_json["cal_weight"] = 846.3;
        config_json["factor"] = 40.34;
        config_json["timeout"] = 15;
        writeConfig();
        ESP.restart();
      }
      else if (SerialBuffer[1] == 0x15)
      {
        Serial1.print("Debug Mode Value Changed");
        preferences.putChar("DEBUG_MODE", SerialBuffer[2]);
      }
      else
      {
        Serial1.println("Unknown Command");
      }
    }
  }

  // if (Serial2.available() && !MENU_MODE) // RFID 태그
  // {
  //   RFIDeventFlag = true;
  // }

  if (RFIDeventFlag == true)
  {
    mcp.digitalWrite(LED_STRIP, LOW);
    bool BuzzBuzzer = true;
    xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
    bool BlinkStatusLED = true;
    xQueueSend(StatusLEDTaskQueue, &BlinkStatusLED, portMAX_DELAY);
    Serial2.readBytes(rfid_buffer, 12);
    Serial2.end();
    lcd.clear();
    lcd.setCursor(0, 0);
    for (int i = 0; i < 10; i++)
    {
      lcd.write(rfid_buffer[i]);
    }
    lcd.setCursor(0, 1);
    lcd.print("TAGGED");
    unsigned char pc_message[18] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '|', '0', '|', '0', '\r', '\n'}; // RFIDRFIDRF|0|0|0CRLF
    for (int i = 0; i < 10; i++)
    {
      pc_message[i] = rfid_buffer[i];
    }
    SendData(true, "RFID Tagged", pc_message, sizeof(pc_message));
    unsigned long WAIT_PRV_MILLIS = millis();
    while (1)
    {
      if (millis() >= WAIT_PRV_MILLIS + (scaleconfig.INPUT_TIMEOUT * 1000) - 1000) // 투입 타임아웃
      {
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("TIMEOUT");
        unsigned char pc_timeout_message[22] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '.', '0', '0', '0', '|', '1', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0.000|1|0
        for (int i = 0; i < 10; i++)
        {
          pc_timeout_message[i] = rfid_buffer[i];
        }
        SendData(true, "Input Timeout", pc_timeout_message, sizeof(pc_timeout_message));
        delay(3000);
        scale_init();
        break;
      }
      if (read_kg() > SCALE_LOW_LIMIT) // 무게 감지
      {
        MeasureFlag = true;
        break;
      }
    }
  }

  if (RFIDeventFlag == true && MeasureFlag == true) // 실제 무게 측량 처리
  {
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("MEASURING");
    lcd.setCursor(0, 1);
    lcd.print("STBL");
    double weight_stbl = stableMode();
    ButtonBlock = true;
    if (!NET_BTN_STATUS)
    {
      weight_stbl = weight_stbl - scaleconfig.NET_WEIGHT;
    }
    if (weight_stbl < SCALE_LOW_LIMIT || weight_stbl > SCALE_HIGH_LIMIT) // 예외처리
    {
      unsigned char pc_message[22] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '.', '0', '0', '0', '|', '1', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0.000|0|0CRLF
      for (int i = 0; i < 10; i++)
      {
        pc_message[i] = rfid_buffer[i];
      }
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Weight Exception");
      SendData(true, "Weight Exception", pc_message, sizeof(pc_message));
    }
    else
    {
      char weight_char[6];
      dtostrf(weight_stbl / 1000, 5, 3, weight_char); // Convert to Kg
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print(weight_char);
      lcd.setCursor(5, 0);
      lcd.print("KG");
      unsigned char pc_message[22] = {'0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '|', '0', '.', '0', '0', '0', '|', '0', '|', '0', 0x0D, 0x0A}; // RFIDRFIDRF|0.000|0|0CRLF
      for (int i = 0; i < 10; i++)
      {
        pc_message[i] = rfid_buffer[i];
      }
      for (int i = 11; i < 16; i++)
      {
        pc_message[i] = weight_char[i - 11];
      }
      if (!USER1_BTN_STATUS)
        pc_message[19] = '1';
      else if (!USER2_BTN_STATUS)
        pc_message[19] = '2';
      else if (!USER3_BTN_STATUS)
        pc_message[19] = '3';
      else if (!USER4_BTN_STATUS)
        pc_message[19] = '4';
      else
        pc_message[19] = '0';
      SendData(true, "Measure End", pc_message, sizeof(pc_message));
      digitalWrite(MOTOR1, HIGH);
      digitalWrite(MOTOR2, LOW);
      delay(MOTOR_HIGH_S);
      digitalWrite(MOTOR1, LOW);
      digitalWrite(MOTOR2, HIGH);
      delay(MOTOR_LOW_S);
      digitalWrite(MOTOR1, LOW);
      digitalWrite(MOTOR2, LOW);
    }
    scale_init();
  }
}