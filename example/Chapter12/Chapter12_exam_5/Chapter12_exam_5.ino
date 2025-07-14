/***********************************************************************
 * Project      :     Tenergy32Gateway RTC DateTime sync NTP Server
 * Description  :     Example program for reading and displaying real-time date/time from DS3231 RTC
 * Hardware     :     Tenergy32GateWay
 * Author       :     Tenergy Innovation Co., Ltd.
 * Date         :     14/07/2025
 * Revision     :     1.0
 * Rev1.0       :     Original
 * website      :     http://www.tenergyinnovation.co.th
 * Email        :     uten.boonliam@tenergyinnovation.co.th
 * TEL          :     +66 89-140-7205
 ***********************************************************************/

#include <Arduino.h>          // ไลบรารีหลักของ Arduino
#include <tenergy32gateway.h> // ไลบรารีควบคุมบอร์ด Tenergy32Gateway
#include <esp_task_wdt.h>     // ไลบรารีสำหรับ Watchdog Timer
#include <esp_system.h>       // สำหรับ esp_read_mac (ไม่ได้ใช้ในตัวอย่างนี้)
#include <WiFi.h>             // ไลบรารี WiFi สำหรับ ESP32
#include <time.h>             // ไลบรารีสำหรับ NTP

/**************************************/
/*        define object variable      */
/**************************************/
Tenergy32GateWay mcu; // สร้างอ็อบเจกต์ mcu สำหรับควบคุมบอร์ด Tenergy32Gateway

/***********************************************************************
 * FUNCTION:    header_print
 * DESCRIPTION: แสดงข้อมูลส่วนหัวของโปรแกรม
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
// ฟังก์ชันแสดงข้อมูลส่วนหัวของโปรแกรมผ่าน Serial Monitor
void header_print(void)
{
    Serial.printf("\r\n***********************************************************************\r\n");
    Serial.printf("* Project      :     Tenergy32Gateway RTC DateTime sync NTP Server\r\n");
    Serial.printf("* Description  :     Example program for reading and displaying real-time date/time from DS3231 RTC\r\n");
    Serial.printf("* Hardware     :     Tenergy32GateWay\r\n");
    Serial.printf("* Author       :     Tenergy Innovation Co., Ltd.\r\n");
    Serial.printf("* Date         :     14/07/2025\r\n");
    Serial.printf("* Revision     :     %s\r\n", mcu._version.c_str());
    Serial.printf("* website      :     http://www.tenergyinnovation.co.th\r\n");
    Serial.printf("* Email        :     uten.boonliam@tenergyinnovation.co.th\r\n");
    Serial.printf("* TEL          :     +66 89-140-7205\r\n");
    Serial.printf("***********************************************************************/\r\n");

    // กำหนด WiFi SSID และ Password (แก้ไขให้ตรงกับ WiFi ที่ต้องการเชื่อมต่อ)
#define WIFI_SSID "YOUR_WIFI_SSID"
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD"

// กำหนด Timezone (UTC+7 สำหรับไทย)
#define GMT_OFFSET_SEC 7 * 3600
#define DAYLIGHT_OFFSET 0
#define NTP_SERVER "pool.ntp.org"
}

/***********************************************************************
 * FUNCTION:    setup
 * DESCRIPTION: setup process
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
// ฟังก์ชัน setup() จะถูกรันครั้งเดียวเมื่อบอร์ดเริ่มทำงาน
void setup()
{
    Serial.begin(115200); // เริ่มต้น Serial Monitor ที่ baudrate 115200
    header_print();       // แสดงข้อมูลส่วนหัวของโปรแกรม

    // เริ่มต้นบอร์ด Tenergy32Gateway (รีเทิร์น false หากล้มเหลว)
    if (!mcu.begin())
    {
        Serial.println("Board initialization failed!");
        while (1)
            ; // ค้างไว้หากบอร์ดไม่พร้อมใช้งาน
    }
    // หน่วงเวลาเพื่อให้เห็นข้อความเริ่มต้น
    delay(1000);

    // --- เชื่อมต่อ WiFi ---
    Serial.print("Connecting to WiFi");
    mcu.displayOLED("Connecting WiFi..."); // แสดงข้อความบน OLED
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    int wifiTimeout = 0;
    while (WiFi.status() != WL_CONNECTED && wifiTimeout < 30)
    {
        delay(500);
        Serial.print(".");
        wifiTimeout++;
    }
    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("\nWiFi connected!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
        mcu.displayOLED(WiFi.localIP().toString().c_str()); // แสดง IP บน OLED
        vTaskDelay(1000);                                   // หน่วงเวลา 1 วินาทีเพื่อให้เห็นข้อความ
    }
    else
    {
        Serial.println("\nWiFi connect failed!");
        mcu.displayOLED("WiFi connect failed!");
        return;
    }

    // --- ดึงเวลาจาก NTP Server ---
    configTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET, NTP_SERVER);
    Serial.println("Waiting for NTP time sync...");
    mcu.displayOLED("Waiting for NTP time sync...");
    struct tm timeinfo;
    int ntpWait = 0;
    while (!getLocalTime(&timeinfo) && ntpWait < 20)
    {
        Serial.print(".");
        delay(500);
        ntpWait++;
    }
    if (getLocalTime(&timeinfo))
    {
        Serial.println("\nNTP time received!");
        // ปรับเวลาเข้า DS3231 RTC
        uint16_t y = timeinfo.tm_year + 1900;
        uint8_t m = timeinfo.tm_mon + 1;
        uint8_t d = timeinfo.tm_mday;
        uint8_t h = timeinfo.tm_hour;
        uint8_t min = timeinfo.tm_min;
        uint8_t s = timeinfo.tm_sec;
        mcu.setDateTime(y, m, d, h, min, s);
        Serial.printf("RTC synced: %04d/%02d/%02d %02d:%02d:%02d\n", y, m, d, h, min, s);
        mcu.displayOLED("RTC synced from NTP!");
        delay(1500);
    }
    else
    {
        Serial.println("\nNTP sync failed!");
        mcu.displayOLED("NTP sync failed!");
        delay(1500);
    }
    WiFi.disconnect(true); // ตัด WiFi ออกเพื่อประหยัดพลังงาน

    // กำหนดและเริ่มต้น Watchdog Timer (รีเซ็ต MCU หากโปรแกรมค้าง)
    esp_task_wdt_init(10, true); // ตั้ง watchdog timeout 10 วินาที
    esp_task_wdt_add(NULL);      // เพิ่ม task ปัจจุบันเข้า watchdog
}

/***********************************************************************
 * FUNCTION:    loop
 * DESCRIPTION: loop process
 * PARAMETERS:  nothing
 * RETURNED:    nothing
 ***********************************************************************/
// ฟังก์ชัน loop() จะถูกรันซ้ำ ๆ ตลอดเวลาขณะบอร์ดทำงาน
void loop()
{
    // อ่านวันที่และเวลาจาก DS3231 RTC และแสดงผล
    uint16_t year;
    uint8_t month, day, hour, minute, second;
    mcu.getDateTime(year, month, day, hour, minute, second);
    char datetimeStr[32];
    snprintf(datetimeStr, sizeof(datetimeStr), "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);
    Serial.println(datetimeStr);
    mcu.displayOLED(datetimeStr);
    esp_task_wdt_reset();
    delay(500);
}
