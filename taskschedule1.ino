/* 
 * 
 * Sketch name : taskschedule1
 * 2026.03.04 by Halin
 * 將按鍵任務套入範本
 * 接下來以本範本加入其他任務
 * 
 * 
*/
#define simp1306  //use SSD1306Ascii.h
//#define std1306 //use U8g2lib.h
//#define st7789  //use TFT_eSPI.h

#include <Wire.h>
#ifdef simp1306
    #include "SSD1306Ascii.h"
    #include "SSD1306AsciiWire.h"
#endif
#ifdef std1306    
     #include <U8g2lib.h>
#endif
#ifdef st7789
     #include <SPI.h>
     #include <TFT_eSPI.h> //有修改增加Adafruit_ST7789.h的顏色定義2026.03.01    
#endif
 
#include <WiFi.h>
#include <WiFiManager.h>
#include <WebServer.h>
#include <ArduinoJson.h>
#include "SPIFFS.h"
#include "time.h"
#include <AHT20.h>

#define serialDSP

// 定義腳位
#define TTP223_PIN 2
#define BUZZER_PIN 3

AHT20 aht;
#ifdef st7789
TFT_eSPI tft;
// 定義腳位
#define TFT_CS    8
#define TFT_DC    9
#define TFT_RST   10
#define TFT_BL    7
#endif

#ifdef simp1306
SSD1306AsciiWire oled;
#endif

#ifdef std1306
U8G2_SSD1306_128X64_NONAME_F_HW_I2C display(U8G2_R0, /* clock=*/ SCL, /* data=*/ SDA, /* reset=*/ U8X8_PIN_NONE);    //Low spped I2C
char st[20];
#endif

//////////受排程任務數//////////
#define MAX_TASKS 2
//////////////////////////////

WebServer server(80);
//////////////////////////////
//全域變數區
//////////////////////////////
float humidity, temp;

//========按鍵參數區===============
#define SCAN_INTERVAL_MS 5
#define DEBOUNCE_MS 50
#define LONG_PRESS_MS 3000
#define CLICK_TIMEOUT_MS 300
enum ButtonEvent {
    EVENT_SINGLE,
    EVENT_DOUBLE,
    EVENT_TRIPLE,
    EVENT_LONG
};
QueueHandle_t buttonEventQueue;

//////////////////////////////
// NTP
//////////////////////////////
const char* ntpServer1 = "pool.ntp.org";
const char* ntpServer2 = "tick.stdtime.gov.tw";
const long gmtOffset_sec = 8 * 3600;
const int daylightOffset_sec = 0;

//////////////////////////////
// 排程結構 24小時 x 7天
//////////////////////////////
struct TaskSchedule {
  TaskHandle_t handle; //讓排程任務控制
  bool table[7][24];
  String name;         //任務名稱
};

TaskSchedule tasks[MAX_TASKS];  //任務排程陣列

//////////////////////////////////////////////////////
// Scheduler任務排程控制
//////////////////////////////////////////////////////
void schedulerTask(void *pv)
{
  struct tm timeinfo;

  while (1)
  {
    if (!getLocalTime(&timeinfo))
    {
      vTaskDelay(pdMS_TO_TICKS(10000));
      continue;
    }

    int weekday = timeinfo.tm_wday;
    int hour = timeinfo.tm_hour;

    for (int t = 0; t < MAX_TASKS; t++)
    {
     if (tasks[t].handle == NULL) continue;
     
     if (tasks[t].table[weekday][hour])
      {
        if (eTaskGetState(tasks[t].handle) == eSuspended)
          vTaskResume(tasks[t].handle);
      }
      else
      {
        if (eTaskGetState(tasks[t].handle) != eSuspended)
          vTaskSuspend(tasks[t].handle);
      }
    }

    vTaskDelay(pdMS_TO_TICKS(30000));
  }
}

//////////////////////////////////////////////////////
// 排程儲存
//////////////////////////////////////////////////////
void saveSchedule()
{
  DynamicJsonDocument doc(8192);
  JsonArray root = doc.createNestedArray("tasks");

  for (int t = 0; t < MAX_TASKS; t++)
  {
    JsonObject obj = root.createNestedObject();
    obj["name"] = tasks[t].name;
    JsonArray week = obj.createNestedArray("week");

    for (int d = 0; d < 7; d++)
    {
      JsonArray day = week.createNestedArray();
      for (int h = 0; h < 24; h++)
        day.add(tasks[t].table[d][h]);
    }
  }

  File file = SPIFFS.open("/schedule.json", FILE_WRITE);
  serializeJson(doc, file);
  file.close();
}

//初始化排程表
void initDefaultSchedule()
{
//清除排程表  
  for (int t = 0; t < MAX_TASKS; t++)
    for (int d = 0; d < 7; d++)
      for (int h = 0; h < 24; h++)
        tasks[t].table[d][h] = false;

  // 範例: Task0 平日9~17
  for (int d = 1; d <= 5; d++)    //星期1到5
    for (int h = 9; h < 17; h++)  //9點到17點
      tasks[0].table[d][h] = true; //設為啟動
}
//////////////////////////////////////////////////////
// 載入排程
//////////////////////////////////////////////////////
void loadSchedule()
{
  if (!SPIFFS.exists("/schedule.json"))
  {
    initDefaultSchedule();
    saveSchedule();  // 🔥 關鍵
    return;
  }  

  File file = SPIFFS.open("/schedule.json");
  DynamicJsonDocument doc(8192);
  deserializeJson(doc, file);
  
//        Serial.println("--- 完整的 JSON 結構如下 ---");
//        serializeJsonPretty(doc, Serial); 
//        Serial.println("\n---------------------------");

  JsonArray root = doc["tasks"];

  for (int t = 0; t < MAX_TASKS; t++)
    for (int d = 0; d < 7; d++)
      for (int h = 0; h < 24; h++)
        tasks[t].table[d][h] = root[t]["week"][d][h];

  file.close();
}

//////////////////////////////////////////////////////
// Web API
//////////////////////////////////////////////////////

void handleRoot()
{
//  File file = SPIFFS.open("/index.html", "r");
  File file = SPIFFS.open("/schedule.html", "r");
  server.streamFile(file, "text/html");
  file.close();
}

void handleGet()
{
  DynamicJsonDocument doc(4096);
  JsonArray root = doc.createNestedArray("tasks");

  for (int t = 0; t < MAX_TASKS; t++)
  {
    JsonObject obj = root.createNestedObject();
    obj["name"] = tasks[t].name;

    JsonArray week = obj.createNestedArray("week");

    for (int d = 0; d < 7; d++)
    {
      JsonArray day = week.createNestedArray();
      for (int h = 0; h < 24; h++)
        day.add(tasks[t].table[d][h]);
    }
  }

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void handleSet()
{
  String body = server.arg("plain");

  DynamicJsonDocument doc(8192);
  deserializeJson(doc, body);

  JsonArray root = doc["tasks"];

  for (int t = 0; t < MAX_TASKS; t++)
    for (int d = 0; d < 7; d++)
      for (int h = 0; h < 24; h++)
        tasks[t].table[d][h] = root[t]["week"][d][h];

  saveSchedule();
  // 🔥 直接回傳最新排程
  handleGet();
}

void handleStatus()
{
  DynamicJsonDocument doc(2048);
  JsonArray arr = doc.to<JsonArray>();

  for (int t = 0; t < MAX_TASKS; t++)
  {
    JsonObject obj = arr.createNestedObject();
    bool running = (eTaskGetState(tasks[t].handle) != eSuspended);
    obj["running"] = running;
    obj["next"] = "auto";
  }

  String out;
  serializeJson(doc, out);
  server.send(200, "application/json", out);
}

void listSPIFFSFiles() {
#ifdef serialDSP      
    Serial.println("目前 SPIFFS 檔案清單：");
#endif    
    File root = SPIFFS.open("/");
    File file = root.openNextFile();
    while (file) {
#ifdef serialDSP      
        Serial.printf("檔案: %s, 大小: %d bytes\n", file.name(), file.size());
#endif        
        file = root.openNextFile();
    }
}

// 取得目前日期字串 (YYYYMMDD)
String getTodayDate() {
    struct tm timeinfo;
    if(!getLocalTime(&timeinfo)){
#ifdef serialDSP      
        Serial.println("無法取得網路時間");
#endif        
        return "00000000";
    }
    char dateStr[9];
    strftime(dateStr, 9, "%Y%m%d", &timeinfo);
    return String(dateStr);
}

//****************************************************
// 開始更改部分
//****************************************************
// 1. TTP223 觸摸任務
void ttp223Task(void *pv) {
  pinMode(TTP223_PIN, INPUT);
/*
    UBaseType_t uxHighWaterMark;      //monitor stack
// 初始量測 
    uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("ButtonTask Start - Free Stack: %ld words\n", uxHighWaterMark);
*/
    bool lastStable = LOW;
    bool lastRead   = LOW;

    TickType_t debounceTick = 0;
    TickType_t pressTick = 0;
    TickType_t releaseTick = 0;

    int clickCount = 0;

    while (1)
    {
        bool state = digitalRead(TTP223_PIN);
        TickType_t now = xTaskGetTickCount();

        if (state != lastRead)
        {
            debounceTick = now;
            lastRead = state;
        }

        if ((now - debounceTick) >= pdMS_TO_TICKS(DEBOUNCE_MS))
        {
            if (state != lastStable)
            {
                lastStable = state;

                if (state == HIGH) // 按下
                {
                    pressTick = now;
                }
                else // 放開
                {
                    TickType_t duration = now - pressTick;

                    if (duration >= pdMS_TO_TICKS(LONG_PRESS_MS))
                    {
                        ButtonEvent evt = EVENT_LONG;
                        xQueueSend(buttonEventQueue, &evt, 0);
                        clickCount = 0;
                    }
                    else
                    {
                        clickCount++;
                        releaseTick = now;
                    }
                }
            }
        }

        if (clickCount > 0 &&
            (now - releaseTick >= pdMS_TO_TICKS(CLICK_TIMEOUT_MS)))
        {
            ButtonEvent evt;

            if (clickCount == 1) evt = EVENT_SINGLE;
            else if (clickCount == 2) evt = EVENT_DOUBLE;
            else evt = EVENT_TRIPLE;

            xQueueSend(buttonEventQueue, &evt, 0);
            clickCount = 0;
        }

/*
        // 2. 定期檢查高水位線 
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);        
        // 如果剩餘空間過小（例如少於 20 words），發出警告
        if(uxHighWaterMark < 20) {
            Serial.printf("WARNING: Stack running low! Only %ld words left.\n", uxHighWaterMark);
        } else {
            Serial.printf("Current ButtonTask: %ld words\n", uxHighWaterMark);
        }
*/

        vTaskDelay(pdMS_TO_TICKS(SCAN_INTERVAL_MS));
    }
}

void buttonEventTask(void *pvParameters)
{
/*
    UBaseType_t uxHighWaterMark;      //monitor stack
    // 初始量測 
    uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);
    Serial.printf("ButtonEventTask Start - Free Stack: %ld words\n", uxHighWaterMark);
*/

    ButtonEvent evt;
    while (1)
    {
        if (xQueueReceive(buttonEventQueue, &evt, portMAX_DELAY))
        {
            switch (evt)
            {
                case EVENT_SINGLE:
#ifdef serialDSP      
                Serial.println("單擊");
#endif                
/*
//================================
                    if (currentState == STATE_MENU) {
                      selectidx++;
                      if(selectidx >= CITY_COUNT){selectidx = 0;}
                      updatemenu();
                      
#ifdef serialDSP      
                      Serial.print("Menu: ");
                      Serial.println(cityList[selectidx].name);
#endif                      
                    } else {
                      pageidx++;
                      if(pageidx >= 2){pageidx =0;}
                      playBeep();
                      xTaskNotifyGive(weatherTaskHandle);  // 立即喚醒氣象任務
                    }
//================================                                                       
*/
                    break;

                case EVENT_DOUBLE:
#ifdef serialDSP      
                    Serial.println("雙擊");
#endif
/*
//===========================================================                    
                    if (currentState == STATE_MAIN) {
                    
#ifdef serialDSP      
                    Serial.println("Manual Weather Update");
#endif                    
                    } else if(currentState == STATE_MENU){
                    myLat = cityList[selectidx].lat;
                    myLon = cityList[selectidx].lon;
                    cityindex = selectidx;
                    cityChanged = true;
                    currentState = STATE_MAIN;
#ifdef serialDSP      
                    Serial.print("Selected: ");
                    Serial.println(cityList[selectidx].name);
                    Serial.println("Back to MAIN");                      
#endif                    
                    }
                    playBeep();
                    xTaskNotifyGive(weatherTaskHandle);  // 立即喚醒氣象任務
//===========================================================                    
*/
                    break;

                case EVENT_TRIPLE:
#ifdef serialDSP      
                        Serial.println("三擊");
#endif
/*                        
//================================
                    if (currentState == STATE_MAIN) {
                        currentState = STATE_MENU;
                        selectidx = cityindex;
                        
                        updatemenu();
#ifdef serialDSP      
                        Serial.println("Enter MENU");
                        Serial.println(cityList[selectidx].name);                      
#endif                        
                    } else {      //在設定選單中執行退出選單
                        currentState = STATE_MAIN; 
                        playBeep();
                        xTaskNotifyGive(weatherTaskHandle);  // 立即喚醒氣象任務
                      }
//================================                   
*/
                    break;

                case EVENT_LONG:
#ifdef serialDSP      
                    Serial.println("Entering Deep Sleep");
#endif                    
/*
                    powerOffAnimation();
                    esp_deep_sleep_enable_gpio_wakeup(1 << 2, ESP_GPIO_WAKEUP_GPIO_HIGH);                      
                    esp_deep_sleep_start();
*/                    
                    break;
            }
        }
/*
        // 2. 定期檢查高水位線 
        uxHighWaterMark = uxTaskGetStackHighWaterMark(NULL);        
        // 如果剩餘空間過小（例如少於 20 words），發出警告
        if(uxHighWaterMark < 20) {
            Serial.printf("WARNING: Stack running low! Only %ld words left.\n", uxHighWaterMark);
        } else {
            Serial.printf("Current ButtonEventTask: %ld words\n", uxHighWaterMark);
        }
*/        
    }
}


// 2. AHT20 感測器任務
void aht20Task(void *pv) {
/*  
  if (!aht.begin()) {
    Serial.println("Could not find AHT20");
    vTaskDelete(NULL);
  }
*/  
  while (1) {
    if(aht.getSensor(&humidity, &temp)){
#ifdef simp1306      
     oled.printf("T:%.2f H:%.2f\n", temp, humidity*100);
#endif     
#ifdef serialDSP      
     Serial.printf("Temp: %.2f C, Hum: %.2f %%\n", temp, humidity);
#endif     
    }
    vTaskDelay(pdMS_TO_TICKS(10000)); // 每10秒讀取一次
  }
}
// 3.Web Server 任務 (讓 Server 跑在獨立 Task 中，不擠在 loop)
void webServerTask(void *pv) {
  server.on("/", handleRoot);
  server.on("/get", HTTP_GET, handleGet);
  server.on("/set", HTTP_POST, handleSet);
  server.on("/status", HTTP_GET, handleStatus);
  
  server.begin();
#ifdef serialDSP      
  Serial.println("🌐 Web Server is Always ON");
#endif  
//  playBeep();
  
while (1) {
    server.handleClient();        // ⚠ 必須輪詢
    vTaskDelay(pdMS_TO_TICKS(5)); // 給予系統微小喘息
  }
}

// 蜂鳴器提示音
void playBeep() {
    tone(BUZZER_PIN, 4000);
    vTaskDelay(pdMS_TO_TICKS(100));
    noTone(BUZZER_PIN);
}

#ifdef simp1306
void initSSD1306(){
// 定義 I2C 地址，通常是 0x3C
#define I2C_ADDRESS 0x3C
// 定義 ESP32-C3 Super Mini 的 I2C 接腳
#define SDA_PIN 8
#define SCL_PIN 9
  // 手動指定接腳初始化 I2C
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(400000); // 設置 400kHz 快速度
  // 初始化 OLED
  oled.begin(&Adafruit128x64, I2C_ADDRESS);  
  // 設定字體 (更多字體可在程式庫的 fonts 目錄找到)
  oled.setFont(Adafruit5x7);
  oled.setScrollMode(SCROLL_MODE_AUTO);
  
  oled.clear();
  oled.println("ESP32-C3 Mini");
  oled.println("Task Sche OK!");
  oled.print("taskschedule1\n");  
}
#endif
#ifdef std1306
void initSSD1306(){
  Wire.begin(8, 9);   // (I2C_SDA, I2C_SCL)
  display.begin();
//  if(bootCount == 0){
  display.setFont(u8g2_font_courB12_tr); // choose 10*15 font
  display.clearBuffer();          // clear the internal memory
  display.drawFrame(0,0,display.getDisplayWidth(),display.getDisplayHeight());  
  sprintf(st,"ESP32-C3mini");
  display.drawStr(4,display.getDisplayHeight()/3,st);  
  display.sendBuffer();                    // transfer internal memory to the display 
  delay(2000); 
  sprintf(st,"Task Sche");
  display.drawStr(20,display.getDisplayHeight()/3+17,st);  
  display.sendBuffer();                    // transfer internal memory to the display 
  delay(2000); 
  sprintf(st,"taskschedule1");
  display.drawStr(1,display.getDisplayHeight()/3+34,st);  
  display.sendBuffer();                    // transfer internal memory to the display 
  delay(3000); 
//  }
}
#endif
//////////////////////////////////////////////////////
// Setup
//////////////////////////////////////////////////////
void setup()
{
// 初始化按鈕與蜂鳴器
  pinMode(BUZZER_PIN, OUTPUT);
  
#ifdef serialDSP      
  delay(500);//給serial啟動時間
  Serial.begin(115200);
#endif

#ifndef st7789  
  initSSD1306();
#endif  
    
  SPIFFS.begin(true);
  delay(500);
#ifdef serialDSP      
  listSPIFFSFiles();  //列出SPIFFS裡的檔案有哪一些
#endif  

// WiFiManager 配網
   WiFi.mode(WIFI_STA); // Wi-Fi設置成STA模式；預設模式為STA+AP
   WiFi.setTxPower(WIFI_POWER_8_5dBm);     
   WiFiManager wm;
   wm.setConnectTimeout(30); 
   if (!wm.autoConnect("ESP32_SCHED_AP")) {
        ESP.restart();
      }
#ifdef serialDSP      
  Serial.println("\nWiFi 已連線");
  Serial.print("IP 位址: ");
  Serial.println(WiFi.localIP());
#endif

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);//同步網路時間
  
#ifdef serialDSP      
  String today = getTodayDate();
  Serial.println(today);
#endif  
  
///////////////////////////////////////////
// 以下創建有順序及跨任務關聯,需特別注意
///////////////////////////////////////////  

  loadSchedule();
  
//按鍵通知訊息需要是跨任務通知,所以在setup中建立
  buttonEventQueue = xQueueCreate(5, sizeof(ButtonEvent));                    //建立按鍵通知訊息

  // 建立 FreeRTOS 任務有先後順序
  // 按鍵之後的處理任務
  xTaskCreate(buttonEventTask, "ButtonEventTask", 2048, NULL, 2, NULL);       //
/////////////////////////////////////////////////////////
// 受排程控制任務創建
/////////////////////////////////////////////////////////
  // 1. TTP223 按鍵
  xTaskCreate(ttp223Task, "TTP223_Touch", 3072, NULL, 1, &tasks[0].handle);
  tasks[0].name = "TTP223_Touch";
  // 2. AHT20 
  xTaskCreate(aht20Task, "AHT20_Sensor", 3072, NULL, 1, &tasks[1].handle);
  tasks[1].name = "AHT20_Sensor";
/////////////////////////////////////////////////////////
// 非受排程控制任務創建
/////////////////////////////////////////////////////////  
  // 3. WebServer - 不存 handle 或是 Scheduler 跳過它
  xTaskCreate(webServerTask, "Web_Service", 4096, NULL, 1, NULL);
  // 4. Scheduler 
  xTaskCreate(schedulerTask, "Scheduler", 3072, NULL, 2, NULL);

}

void loop()
{
    vTaskDelay(pdMS_TO_TICKS(10));
}
