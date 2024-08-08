#define NET_BTN 0
#define TARE_BTN 1
#define USER1_BTN 2
#define USER2_BTN 3
#define USER3_BTN 6
#define USER4_BTN 5
#define LED_STRIP 8
#define BUZZER 9
#define USER1_LED 10
#define TARE_LED 11
#define NET_LED 12
#define USER4_LED 13
#define USER3_LED 14
#define USER2_LED 15

#define CONFIG_START 0
#define CONFIG_UP 1
#define CONFIG_DOWN 2
#define CONFIG_BACK 3
#define CONFIG_ENTER 4

extern Adafruit_MCP23X17 mcp;

extern QueueHandle_t BuzzerTaskQueue;
extern QueueHandle_t StatusLEDTaskQueue;
extern QueueHandle_t MenuTaskQueue;

extern TaskHandle_t MenuTaskHandle;

extern bool Show_Loadcell_Value;

extern bool ButtonBlock;

extern bool ButtonBlock;

extern bool NET_BTN_STATUS;
extern bool TARE_BTN_STATUS;
extern bool USER1_BTN_STATUS;
extern bool USER2_BTN_STATUS;
extern bool USER3_BTN_STATUS;
extern bool USER4_BTN_STATUS;

extern bool NET_BTN_CNT;
extern bool TARE_BTN_CNT;
extern bool USER1_BTN_CNT;
extern bool USER2_BTN_CNT;
extern bool USER3_BTN_CNT;
extern bool USER4_BTN_CNT;

extern unsigned char MENU_MODE_CNT;
extern bool MENU_MODE;

extern void scale_tare();
extern void appendLogFile(bool timecode, String message, bool crlf);

void button(unsigned char pin)
{
  bool BlinkStatusLED = true;
  xQueueSend(StatusLEDTaskQueue, &BlinkStatusLED, portMAX_DELAY);
  if (!ButtonBlock)
  {
    switch (pin)
    {
    case NET_BTN:
      if (!MENU_MODE)
      {
        if (!NET_BTN_CNT)
        {
          NET_BTN_CNT++;
          NET_BTN_STATUS = NET_BTN_STATUS ? false : true;
          MENU_MODE_CNT++;
          mcp.digitalWrite(NET_LED, NET_BTN_STATUS);
          bool BuzzBuzzer = true;
          xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
        }
        else
        {
          NET_BTN_CNT = 0;
        }
      }
      break;

    case TARE_BTN:
      if (!MENU_MODE)
      {
        TARE_BTN_STATUS = TARE_BTN_STATUS ? false : true;
        mcp.digitalWrite(TARE_LED, TARE_BTN_STATUS);
        if (!TARE_BTN_CNT)
        {
          TARE_BTN_CNT++;
          bool BuzzBuzzer = true;
          xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
          scale_tare();
        }
        else
        {
          TARE_BTN_CNT = 0;
        }
      }
      break;

    case USER1_BTN:
      if (MENU_MODE)
      {
        USER1_BTN_STATUS = USER1_BTN_STATUS ? false : true;
        mcp.digitalWrite(USER1_LED, USER1_BTN_STATUS);
        if (!USER1_BTN_CNT)
        {
          // menu.up();
          USER1_BTN_CNT++;
          char Button_No = CONFIG_UP;
          xQueueSend(MenuTaskQueue, &Button_No, portMAX_DELAY);
          bool BuzzBuzzer = true;
          xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
        }
        else
        {
          USER1_BTN_CNT = 0;
        }
      }
      else if (!USER1_BTN_CNT)
      {
        USER1_BTN_CNT++;
        USER1_BTN_STATUS = USER1_BTN_STATUS ? false : true;
        USER2_BTN_STATUS = true;
        USER3_BTN_STATUS = true;
        USER4_BTN_STATUS = true;
        mcp.digitalWrite(USER1_LED, USER1_BTN_STATUS);
        mcp.digitalWrite(USER2_LED, USER2_BTN_STATUS);
        mcp.digitalWrite(USER3_LED, USER3_BTN_STATUS);
        mcp.digitalWrite(USER4_LED, USER4_BTN_STATUS);
        bool BuzzBuzzer = true;
        xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
      }
      else
      {
        USER1_BTN_CNT = 0;
        if (MENU_MODE_CNT >= 10) // Enter Menu Mode
        {
          MENU_MODE_CNT = 0;
          MENU_MODE = true;
          NET_BTN_STATUS = true;
          TARE_BTN_STATUS = true;
          USER1_BTN_STATUS = true;
          USER2_BTN_STATUS = true;
          USER3_BTN_STATUS = true;
          USER4_BTN_STATUS = true;
          Serial2.end();
          Show_Loadcell_Value = false;
          //vTaskSuspend(TestTaskHandle);
          mcp.digitalWrite(LED_STRIP, LOW);
          mcp.digitalWrite(NET_LED, NET_BTN_STATUS);
          mcp.digitalWrite(TARE_LED, TARE_BTN_STATUS);
          mcp.digitalWrite(USER1_LED, USER1_BTN_STATUS);
          mcp.digitalWrite(USER2_LED, USER2_BTN_STATUS);
          mcp.digitalWrite(USER3_LED, USER3_BTN_STATUS);
          mcp.digitalWrite(USER4_LED, USER4_BTN_STATUS);
          vTaskResume(MenuTaskHandle);
          char Button_No = CONFIG_START;
          xQueueSend(MenuTaskQueue, &Button_No, portMAX_DELAY);
        }
      }
      break;

    case USER2_BTN:
      if (MENU_MODE)
      {
        USER2_BTN_STATUS = USER2_BTN_STATUS ? false : true;
        mcp.digitalWrite(USER2_LED, USER2_BTN_STATUS);
        if (!USER2_BTN_CNT)
        {
          // menu.back();
          USER2_BTN_CNT++;
          char Button_No = CONFIG_BACK;
          xQueueSend(MenuTaskQueue, &Button_No, portMAX_DELAY);
          bool BuzzBuzzer = true;
          xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
        }
        else
        {
          USER2_BTN_CNT = 0;
        }
      }
      else if (!USER2_BTN_CNT)
      {
        USER2_BTN_CNT++;
        USER2_BTN_STATUS = USER2_BTN_STATUS ? false : true;
        USER1_BTN_STATUS = true;
        USER3_BTN_STATUS = true;
        USER4_BTN_STATUS = true;
        mcp.digitalWrite(USER1_LED, USER1_BTN_STATUS);
        mcp.digitalWrite(USER2_LED, USER2_BTN_STATUS);
        mcp.digitalWrite(USER3_LED, USER3_BTN_STATUS);
        mcp.digitalWrite(USER4_LED, USER4_BTN_STATUS);
        bool BuzzBuzzer = true;
        xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
      }
      else
      {
        USER2_BTN_CNT = 0;
      }
      break;

    case USER3_BTN:
      if (MENU_MODE)
      {
        USER3_BTN_STATUS = USER3_BTN_STATUS ? false : true;
        mcp.digitalWrite(USER3_LED, USER3_BTN_STATUS);
        if (!USER3_BTN_CNT)
        {
          // menu.down();
          USER3_BTN_CNT++;
          char Button_No = CONFIG_DOWN;
          xQueueSend(MenuTaskQueue, &Button_No, portMAX_DELAY);
          bool BuzzBuzzer = true;
          xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
        }
        else
        {
          USER3_BTN_CNT = 0;
        }
      }
      else if (!USER3_BTN_CNT)
      {
        USER3_BTN_CNT++;
        USER3_BTN_STATUS = USER3_BTN_STATUS ? false : true;
        USER1_BTN_STATUS = true;
        USER2_BTN_STATUS = true;
        USER4_BTN_STATUS = true;
        mcp.digitalWrite(USER1_LED, USER1_BTN_STATUS);
        mcp.digitalWrite(USER2_LED, USER2_BTN_STATUS);
        mcp.digitalWrite(USER3_LED, USER3_BTN_STATUS);
        mcp.digitalWrite(USER4_LED, USER4_BTN_STATUS);
        bool BuzzBuzzer = true;
        xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
      }
      else
      {
        USER3_BTN_CNT = 0;
      }
      break;

    case USER4_BTN:
      if (MENU_MODE)
      {
        USER4_BTN_STATUS = USER4_BTN_STATUS ? false : true;
        mcp.digitalWrite(USER4_LED, USER4_BTN_STATUS);
        if (!USER4_BTN_CNT)
        {
          // menu.enter();
          USER4_BTN_CNT++;
          char Button_No = CONFIG_ENTER;
          xQueueSend(MenuTaskQueue, &Button_No, portMAX_DELAY);
          bool BuzzBuzzer = true;
          xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
        }
        else
        {
          USER4_BTN_CNT = 0;
        }
      }
      else if (!USER4_BTN_CNT)
      {
        USER4_BTN_CNT++;
        USER4_BTN_STATUS = USER4_BTN_STATUS ? false : true;
        USER1_BTN_STATUS = true;
        USER2_BTN_STATUS = true;
        USER3_BTN_STATUS = true;
        mcp.digitalWrite(USER4_LED, USER4_BTN_STATUS);
        mcp.digitalWrite(USER1_LED, USER1_BTN_STATUS);
        mcp.digitalWrite(USER2_LED, USER2_BTN_STATUS);
        mcp.digitalWrite(USER3_LED, USER3_BTN_STATUS);
        mcp.digitalWrite(USER4_LED, USER4_BTN_STATUS);
        bool BuzzBuzzer = true;
        xQueueSend(BuzzerTaskQueue, &BuzzBuzzer, portMAX_DELAY);
      }
      else
      {
        USER4_BTN_CNT = 0;
      }
      break;
    }
  }
}