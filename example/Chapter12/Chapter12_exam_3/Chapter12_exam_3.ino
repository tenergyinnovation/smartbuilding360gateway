
/***********************************************************************
 * Project      :     Tenergy32Gateway RTC DateTime Demo
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
    Serial.printf("* Project      :     Tenergy32Gateway RTC DateTime Demo\r\n");
    Serial.printf("* Description  :     Example program for reading and displaying real-time date/time from DS3231 RTC\r\n");
    Serial.printf("* Hardware     :     Tenergy32GateWay\r\n");
    Serial.printf("* Author       :     Tenergy Innovation Co., Ltd.\r\n");
    Serial.printf("* Date         :     14/07/2025\r\n");
    Serial.printf("* Revision     :     %s\r\n", mcu._version.c_str());
    Serial.printf("* website      :     http://www.tenergyinnovation.co.th\r\n");
    Serial.printf("* Email        :     uten.boonliam@tenergyinnovation.co.th\r\n");
    Serial.printf("* TEL          :     +66 89-140-7205\r\n");
    Serial.printf("***********************************************************************/\r\n");
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

    // กำหนดและเริ่มต้น Watchdog Timer (รีเซ็ต MCU หากโปรแกรมค้าง)
    esp_task_wdt_init(10, true); // ตั้ง watchdog timeout 10 วินาที
    esp_task_wdt_add(NULL);      // เพิ่ม task ปัจจุบันเข้า watchdog

    // เริ่มต้นบอร์ด Tenergy32Gateway (รีเทิร์น false หากล้มเหลว)
    if (!mcu.begin())
    {
        Serial.println("Board initialization failed!");
        while (1)
            ; // ค้างไว้หากบอร์ดไม่พร้อมใช้งาน
    }
    // หน่วงเวลาเพื่อให้เห็นข้อความเริ่มต้น
    delay(1000);

    // ไม่ต้องปิดรีเลย์ทั้งหมดในตัวอย่างนี้
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
    // ประกาศตัวแปรสำหรับเก็บวันที่และเวลา
    uint16_t year;
    uint8_t month, day, hour, minute, second;

    // อ่านวันที่และเวลาจาก DS3231 RTC ผ่านไลบรารี
    mcu.getDateTime(year, month, day, hour, minute, second);

    // เตรียมข้อความสำหรับแสดงผล
    char datetimeStr[32];
    snprintf(datetimeStr, sizeof(datetimeStr), "%04d/%02d/%02d %02d:%02d:%02d", year, month, day, hour, minute, second);

    // แสดงผลวันที่และเวลาผ่าน Serial Monitor
    Serial.println(datetimeStr);

    // แสดงผลวันที่และเวลาผ่าน OLED (บรรทัดเดียว)
    mcu.displayOLED(datetimeStr);

    // รีเซ็ต Watchdog Timer และหน่วงเวลาเล็กน้อย
    esp_task_wdt_reset();
    delay(500);
}
