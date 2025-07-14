/***********************************************************************
 * Project      :     SmartBuilding360gateway show status of switch
 * Description  :     Test program for Tenergy32 Gateway board
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
    Serial.printf("* Project      :     SmartBuilding360gateway show status of switch\r\n");
    Serial.printf("* Description  :     Show status of switch for SmartBuilding360gateway\r\n");
    Serial.printf("* Hardware     :     SmartBuilding360gateway\r\n");
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
    // อ่านสถานะปุ่ม SW1, SW2 และ DIP Switch (SW3) จากบอร์ด
    bool sw1 = mcu.readSW1();         // อ่านสถานะปุ่ม SW1 (true = กด)
    bool sw2 = mcu.readSW2();         // อ่านสถานะปุ่ม SW2 (true = กด)
    bool dip = mcu.readSlideSwitch(); // อ่านสถานะ DIP Switch (true = ON)

    // ประกาศตัวแปรสำหรับเก็บข้อความแต่ละบรรทัดบน OLED
    char line1[40], line2[40], line3[40], line4[40];

    // แปลงสถานะเป็นข้อความเพื่อแสดงผล
    snprintf(line1, sizeof(line1), "SW1: %s", sw1 ? "PRESSED" : "RELEASED");
    snprintf(line2, sizeof(line2), "SW2: %s", sw2 ? "PRESSED" : "RELEASED");
    snprintf(line3, sizeof(line3), "DIP: %s", dip ? "ON" : "OFF");

    // แสดงผลสถานะปุ่มและ DIP Switch ผ่าน Serial Monitor
    Serial.printf("%s | %s | %s\r\n", line1, line2, line3);

    // แสดงผลสถานะปุ่มและ DIP Switch ผ่านหน้าจอ OLED (3 บรรทัด)
    mcu.displayOLEDLines(line1, line2, line3);

    // รีเซ็ต Watchdog Timer เพื่อป้องกัน MCU รีเซ็ตโดยไม่จำเป็น
    esp_task_wdt_reset();
    delay(500); // อัปเดตข้อมูลทุก 0.5 วินาที
}
