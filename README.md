# Task_Scheduler
<b>基於 ESP32（ESP32-C3）與 FreeRTOS 的多任務排程框架，範例為按鍵事件與感測器讀取、Web API 控制與時間排程系統</b><br>
<br>
🚀 [專案特色]<br>
🧠 FreeRTOS 多任務架構<br>
⏰ 7天 × 24小時 任務排程系統<br>
🌐 內建 Web Server（可遠端設定排程）<br>
📶 WiFiManager 自動配網<br>
💾 SPIFFS 儲存排程設定（JSON）<br>
👆 TTP223 觸控按鍵（支援單擊 / 雙擊 / 三擊 / 長按）<br>
🌡️ AHT20 溫濕度感測<br>
📺 OLED 顯示（SSD1306 / U8g2 可切換）<br>
🔊 蜂鳴器提示音<br>
<br>
🧩 [系統架構]<br>
                +----------------------+<br>
                |   Web Interface      |<br>
                |  /get /set /status   |<br>
                +----------+-----------+<br>
                           |<br>
                           v<br>
+------------+     +------------------+     +------------------+<br>
|  Scheduler | --> | Task Control     | --> | FreeRTOS Tasks   |<br>
|  (NTP)     |     | (Suspend/Resume) |     |                  |<br>
+------------+     +------------------+     +------------------+<br>
                                                |        |<br>
                                                v        v<br>
                                        Task1(TTP223) Task2(AHT20)<br>

📂 [專案結構]<br>
.<br>
├── taskschedule1.ino<br>
├── data/<br>
│   └── schedule.html   # Web UI<br>
<br>
⚙️ [硬體需求]<br>
元件	說明<br>
ESP32-C3 Super Mini	主控板<br>
TTP223	觸摸按鍵  (範例Task1)<br>
AHT20	溫濕度感測器 (範例Task2)<br>
SSD1306 OLED	顯示器（I2C）<br>
蜂鳴器	提示音<br>
<br>
🧠 [任務說明]<br>
1️⃣ Scheduler Task<br>
每 30 秒檢查一次時間<br>
根據排程表控制任務：<br>
vTaskResume()<br>
vTaskSuspend()<br>
<br>
2️⃣ TTP223 按鍵任務<br>
支援事件：<br>
動作	說明<br>
單擊	EVENT_SINGLE<br>
雙擊	EVENT_DOUBLE<br>
三擊	EVENT_TRIPLE<br>
長按	EVENT_LONG<br>
👉 使用 Queue 傳遞事件給 buttonEventTask<br>
<br>
3️⃣ AHT20 感測任務<br>
每 10 秒讀取溫濕度<br>
顯示於 OLED / Serial<br>
<br>
4️⃣ Web Server 任務<br>
API	方法	說明<br>
/	GET	Web UI<br>
/get	GET	取得排程<br>
/set	POST	設定排程<br>
/status	GET	任務狀態<br>
<br>
📅 [排程格式]<br>
{<br>
  "tasks": [<br>
    {<br>
      "name": "Task1",<br>
      "week": [<br>
        [false, true, ...],   // Sunday (0)<br>
        ...<br>
        [false, false, ...]   // Saturday (6)<br>
      ]<br>
    }<br>
  ]<br>
}<br>
<br>
💾 [SPIFFS]<br>
儲存檔案：/schedule.json<br>
開機流程：<br>
檢查檔案是否存在<br>
不存在 → 建立預設排程<br>
存入 SPIFFS<br>
<br>
🌐 [WiFi 設定]<br>
使用 WiFiManager<br>
AP 名稱：ESP32_SCHED_AP<br>
首次開機會進入配網模式<br>
<br>
⏱️ [NTP 時間同步]<br>
pool.ntp.org<br>
tick.stdtime.gov.tw<br>
GMT+8<br>
<br>
🖥️ [顯示器支援]<br>
可切換三種模式：<br>
#define simp1306   // SSD1306Ascii (輕量)<br>
#define std1306    // U8g2 (完整功能)<br>
#define st7789     // TFT (彩色)<br>
<br>
🛠️ [開發重點]<br>
任務透過 TaskHandle_t 控制<br>
使用 eTaskGetState() 判斷狀態<br>
Queue 用於跨任務事件傳遞<br>
WebServer 獨立 Task 避免阻塞<br>
<br>
⚠️ [注意事項]<br>
*WebServer 必須持續呼叫：<br>
*server.handleClient();<br>
*SPIFFS 建議搭配 data/ 上傳工具<br>
*任務 Stack Size 請依實際需求調整<br>
<br>
<br>
