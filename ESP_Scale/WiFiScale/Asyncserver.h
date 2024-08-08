#include "configs.h"
#include "WebConfig_html.h"

#define LED1 15 // STATUS
#define LED2 14 // WIFI

extern AsyncWebServer server;
extern WiFiClient client;
extern DynamicJsonDocument config_json;
extern LiquidCrystal_I2C lcd;

extern TaskHandle_t WifiLEDTaskHandle;

extern void appendLogFile(bool timecode, String message, bool crlf);
extern void writeConfig();
extern uint64_t getTotalUsedBytes(const char *path);

extern struct SystemConfig sysconfig;
extern struct ScaleConfig scaleconfig;

extern bool rebooting;
extern bool wifi_enable;

String convertFileSize(uint64_t bytes)
{
  if (bytes < 1024)
  {
    return String(bytes) + " B";
  }
  else if (bytes < 1048576)
  {
    return String(bytes / 1024.0) + " kB";
  }
  else if (bytes < 1073741824)
  {
    return String(bytes / 1048576.0) + " MB";
  }
}

String processor(const String &var)
{
  if (var == "DEVICE_NAME")
  {
    return sysconfig.Device_Name;
  }
  if (var == "SSID")
  {
    return sysconfig.WiFi_SSID;
  }
  if (var == "PASS")
  {
    return sysconfig.WiFi_PASS;
  }
  if (var == "RSSI")
  {
    return String(WiFi.RSSI());
  }
  if (var == "WIFI_QUALITY")
  {
    if (WiFi.status() != WL_CONNECTED)
    {
      return "WiFi Not Connected";
    }
    int rssi = WiFi.RSSI();
    if (rssi > -40)
    {
      return "Very Good";
    }
    else if (rssi > -60)
    {
      return "Good";
    }
    else if (rssi > -70)
    {
      return "Weak";
    }
    else
    {
      return "Poor";
    }
  }
  if (var == "IP")
  {
    if (!wifi_enable || WiFi.status() != WL_CONNECTED)
    {
      return WiFi.softAPIP().toString();
    }
    else
    {
      return WiFi.localIP().toString();
    }
  }
  if (var == "MAC")
  {
    return WiFi.macAddress();
  }
  if (var == "TCP_STATUS")
  {
    if (client.connected())
    {
      return "Connected";
    }
    else
    {
      return "Disconnected";
    }
  }
  if (var == "TCP_IP")
  {
    return sysconfig.TCP_IP;
  }
  if (var == "TCP_PORT")
  {
    return sysconfig.TCP_PORT;
  }
  if (var == "MODE")
  {
    if (sysconfig.COMM_MODE == "RS232")
    {
      return "RS232 Only";
    }
    if (sysconfig.COMM_MODE == "TCP/IP")
    {
      return "RS232+TCP/IP";
    }
  }
  if (var == "BAUD")
  {
    return sysconfig.RS232_BAUD;
  }
  if (var == "FlashSize")
  {
    return convertFileSize(ESP.getFlashChipSize());
  }
  if (var == "SD_Used")
  {
    return convertFileSize(getTotalUsedBytes("/log"));
  }
  if (var == "SD_Total")
  {
    return convertFileSize(SD.totalBytes());
  }
  if (var == "Heap")
  {
    return convertFileSize(ESP.getFreeHeap());
  }
  if (var == "BUILD_VER")
  {
    return sysconfig.build_date;
  }
  if (var == "NET_W")
  {
    return String(scaleconfig.NET_WEIGHT);
  }
  if (var == "CAL_W")
  {
    return String(scaleconfig.CAL_WEIGHT);
  }
  if (var == "SCALE")
  {
    return String(scaleconfig.FACTOR);
  }
  if (var == "TM")
  {
    return String(scaleconfig.INPUT_TIMEOUT);
  }
  return String();
}

void notFound(AsyncWebServerRequest *request)
{
  request->send(404);
}

void setupAsyncServer()
{
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if(!request->authenticate(sysconfig.http_username.c_str(), sysconfig.http_password.c_str()))
    {
      return request->requestAuthentication();
    }
    request->send_P(200, "text/html", manager_html, processor);
  });

  server.on("/wifi", HTTP_POST, [](AsyncWebServerRequest *request) {
  int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isPost()){
      if (p->name() == "WiFi_SSID") {
        String ssid = p->value().c_str();
        Serial.print("SSID set to: ");
        Serial.println(ssid);
        appendLogFile(true, "[SYSTEM] WiFi_SSID Config Changed - ", false);
        appendLogFile(false, ssid, true);
        config_json["ssid"] = ssid;
      }
      if (p->name() == "WiFi_PASS") {
        String pass = p->value().c_str();
        Serial.print("Password set to: ");
        Serial.println(pass);
        appendLogFile(true, "[SYSTEM] WiFi_PASS Config Changed - ", false);
        appendLogFile(false, pass, true);
        config_json["pass"] = pass;
      }
    }
  }
	writeConfig();
  request->redirect("/");
  });

  server.on("/tcpip", HTTP_POST, [](AsyncWebServerRequest *request) {
  int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isPost()){
      if (p->name() == "TCP_IP") {
        String tcp_ip = p->value().c_str();
        Serial.print("IP set to: ");
				Serial.println(tcp_ip);
        appendLogFile(true, "[SYSTEM] TCP_IP Config Changed - ", false);
        appendLogFile(false, tcp_ip, true);
        config_json["tcp_ip"] = tcp_ip;
      }
      if (p->name() == "TCP_PORT") {
        String tcp_port = p->value().c_str();
        Serial.print("Port set to: ");
        Serial.println(tcp_port);
        appendLogFile(true, "[SYSTEM] TCP_PORT Config Changed - ", false);
        appendLogFile(false, tcp_port, true);
        config_json["tcp_port"] = tcp_port;
      }
    }
  }
	writeConfig();
  request->redirect("/");
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    rebooting = !Update.hasError();
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html", rebooting ? ok_html : failed_html);

    response->addHeader("Connection", "close");
    request->send(response);
  },
  [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final)
  {
    if(!index)
	  {
      mcp.digitalWrite(LED_STRIP, LOW);
      vTaskSuspend(WifiLEDTaskHandle);
      digitalWrite(LED1, HIGH);
      digitalWrite(LED2, HIGH);
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("F/W UPDATING...");
      Serial.print("Updating: ");
      Serial.println(filename.c_str());

      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
	    {
        Update.printError(Serial);
      }
    }
    if(!Update.hasError())
	  {
      if(Update.write(data, len) != len)
	    {
        Update.printError(Serial);
      }
    }
    if(final)
	  {
      if(Update.end(true))
	    {
        Serial.print("The update is finished: ");
        Serial.println(convertFileSize(index + len));
      }
	    else
	    {
        Update.printError(Serial);
      }
    }
  });

  server.on("/baud", HTTP_POST, [](AsyncWebServerRequest *request) {
  int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isPost()){
      if (p->name() == "BAUD") {
        String baudrate = p->value().c_str();
        Serial.print("BAUD set to: ");
        Serial.println(baudrate);
        appendLogFile(true, "[SYSTEM] BAUD Config Changed - ", false);
        appendLogFile(false, baudrate, true);
        config_json["baud"] = baudrate;
      }
    }
  }
	writeConfig();
  request->redirect("/");
  });

  server.on("/mode", HTTP_POST, [](AsyncWebServerRequest *request) {
  int params = request->params();
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isPost()){
      if (p->name() == "MODE") {
        String mode = p->value().c_str();
        Serial.print("MODE set to: ");
        Serial.println(mode);
        appendLogFile(true, "[SYSTEM] MODE Config Changed - ", false);
        appendLogFile(false, mode, true);
        config_json["mode"] = mode;
      }
    }
  }
	writeConfig();
  request->redirect("/");
  });

  server.on("/sc_param", HTTP_POST, [](AsyncWebServerRequest *request) {
  int params = request->params();
  String PARAM;
  String VAL;
  for(int i=0;i<params;i++){
    AsyncWebParameter* p = request->getParam(i);
    if(p->isPost()){
      if (p->name() == "PARAM") {
        PARAM = p->value().c_str();
      }
      if (p->name() == "VALUE") {
        VAL = p->value().c_str();
      }
    }
    if (PARAM == "NW") // Net Weight
    {
      appendLogFile(true, "[SYSTEM] Net Weight Config Changed - ", false);
      appendLogFile(false, VAL, true);
      config_json["net_weight"] = VAL.toDouble();
    }
    else if (PARAM == "CW") // Callibration Weight
    {
      appendLogFile(true, "[SYSTEM] Callibration Weight Config Changed - ", false);
      appendLogFile(false, VAL, true);
      config_json["cal_weight"] = VAL.toDouble();
    }
    else if (PARAM == "TM") // Input Timeout
    {
      appendLogFile(true, "[SYSTEM] Input Timeout Config Changed - ", false);
      appendLogFile(false, VAL, true);
      config_json["timeout"] = VAL.toInt();
    }
  }
	writeConfig();
  request->redirect("/");
  });

  server.on("/configfile", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if(!request->authenticate(sysconfig.http_username.c_str(), sysconfig.http_password.c_str()))
    {
      return request->requestAuthentication();
    }
    request->send(SD, "/config/config.txt", String(), true);
  });

  server.on("/reboot", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if(!request->authenticate(sysconfig.http_username.c_str(), sysconfig.http_password.c_str()))
    {
      return request->requestAuthentication();
    }
    request->redirect("/");
    delay(200);
    ESP.restart();
  });

  server.serveStatic("/getlog", SD, "/log/").setDefaultFile("file_do_not_exist.txt");
  
  server.onNotFound(notFound);

  server.begin();
}