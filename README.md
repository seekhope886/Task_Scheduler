# Task_Scheduler
<b>基於 ESP32（ESP32-C3）與 FreeRTOS 的多任務排程框架，範例為按鍵事件與感測器讀取、Web API 控制與時間排程系統</b>

🚀 [專案特色]
🧠 FreeRTOS 多任務架構
⏰ 7天 × 24小時 任務排程系統
🌐 內建 Web Server（可遠端設定排程）
📶 WiFiManager 自動配網
💾 SPIFFS 儲存排程設定（JSON）
👆 TTP223 觸控按鍵（支援單擊 / 雙擊 / 三擊 / 長按）
🌡️ AHT20 溫濕度感測
📺 OLED 顯示（SSD1306 / U8g2 可切換）
🔊 蜂鳴器提示音

🧩 [系統架構]
                +----------------------+
                |   Web Interface      |
                |  /get /set /status   |
                +----------+-----------+
                           |
                           v
+------------+     +------------------+     +------------------+
|  Scheduler | --> | Task Control     | --> | FreeRTOS Tasks   |
|  (NTP)     |     | (Suspend/Resume) |     |                  |
+------------+     +------------------+     +------------------+
                                                |        |
                                                v        v
                                        Task1(TTP223) Task2(AHT20)

📂 [專案結構]
.
├── taskschedule1.ino
├── data/
│   └── schedule.html   # Web UI

⚙️ [硬體需求]
元件	說明
ESP32-C3 Super Mini	主控板
TTP223	觸摸按鍵  (範例Task1)
AHT20	溫濕度感測器 (範例Task2)
SSD1306 OLED	顯示器（I2C）
蜂鳴器	提示音

🧠 [任務說明]
1️⃣ Scheduler Task
每 30 秒檢查一次時間
根據排程表控制任務：
vTaskResume()
vTaskSuspend()

2️⃣ TTP223 按鍵任務
支援事件：
動作	說明
單擊	EVENT_SINGLE
雙擊	EVENT_DOUBLE
三擊	EVENT_TRIPLE
長按	EVENT_LONG
👉 使用 Queue 傳遞事件給 buttonEventTask

3️⃣ AHT20 感測任務
每 10 秒讀取溫濕度
顯示於 OLED / Serial

4️⃣ Web Server 任務
API	方法	說明
/	GET	Web UI
/get	GET	取得排程
/set	POST	設定排程
/status	GET	任務狀態

📅 [排程格式]
{
  "tasks": [
    {
      "name": "Task1",
      "week": [
        [false, true, ...],   // Sunday (0)
        ...
        [false, false, ...]   // Saturday (6)
      ]
    }
  ]
}

💾 [SPIFFS]
儲存檔案：/schedule.json
開機流程：
檢查檔案是否存在
不存在 → 建立預設排程
存入 SPIFFS

🌐 [WiFi 設定]
使用 WiFiManager
AP 名稱：ESP32_SCHED_AP
首次開機會進入配網模式

⏱️ [NTP 時間同步]
pool.ntp.org
tick.stdtime.gov.tw
GMT+8

🖥️ [顯示器支援]
可切換三種模式：
#define simp1306   // SSD1306Ascii (輕量)
#define std1306    // U8g2 (完整功能)
#define st7789     // TFT (彩色)

🛠️ [開發重點]
任務透過 TaskHandle_t 控制
使用 eTaskGetState() 判斷狀態
Queue 用於跨任務事件傳遞
WebServer 獨立 Task 避免阻塞

⚠️ [注意事項]
*WebServer 必須持續呼叫：
*server.handleClient();
*SPIFFS 建議搭配 data/ 上傳工具
*任務 Stack Size 請依實際需求調整

