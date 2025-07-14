/***********************************************************************
 * Project      :     Tenergy32Gateway Relay Control Demo
 * Description  :     Example program for controlling relays on Tenergy32 Gateway board
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
    Serial.printf("* Project      :     Tenergy32Gateway Relay Control Demo\r\n");
    Serial.printf("* Description  :     Example program for controlling relays on Tenergy32 Gateway board\r\n");
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

    // ปิดรีเลย์ทั้งหมดก่อน
    mcu.relay1Off();
    mcu.relay2Off();
    mcu.relay3Off();
    mcu.relay4Off();
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

    static uint8_t relayStep = 0; // ตัวแปรสำหรับเก็บสถานะรีเลย์ปัจจุบัน
    // เปิดรีเลย์ตามลำดับ
    switch (relayStep)
    {
    case 0:
        mcu.relay1On(); // เปิดรีเลย์ 1
        break;
    case 1:
        mcu.relay2On(); // เปิดรีเลย์ 2
        break;
    case 2:
        mcu.relay3On(); // เปิดรีเลย์ 3
        break;
    case 3:
        mcu.relay4On(); // เปิดรีเลย์ 4
        break;
    case 4:
        mcu.relay1Off(); // ปิดรีเลย์ 1
        mcu.relay2Off(); // ปิดรีเลย์ 2
        mcu.relay3Off(); // ปิดรีเลย์ 3
        mcu.relay4Off(); // ปิดรีเลย์ 4
        break;
    }


    // อ่านสถานะรีเลย์
    bool r1 = mcu.relay1State();
    bool r2 = mcu.relay2State();
    bool r3 = mcu.relay3State();
    bool r4 = mcu.relay4State();

    // เตรียมข้อความแสดงผล
    char line1[40], line2[40], line3[40], line4[40];
    snprintf(line1, sizeof(line1), "Relay1: %s", r1 ? "ON" : "OFF");
    snprintf(line2, sizeof(line2), "Relay2: %s", r2 ? "ON" : "OFF");
    snprintf(line3, sizeof(line3), "Relay3: %s", r3 ? "ON" : "OFF");
    snprintf(line4, sizeof(line4), "Relay4: %s", r4 ? "ON" : "OFF");

    // แสดงผลผ่าน Serial Monitor
    Serial.printf("%s | %s | %s | %s\r\n", line1, line2, line3, line4);

    // แสดงผลผ่าน OLED (4 บรรทัด)
    mcu.displayOLEDLines(line1, line2, line3, line4);

    // รอ 2 วินาที แล้วเปลี่ยนไปเปิดรีเลย์ตัวถัดไป
    esp_task_wdt_reset(); // รีเซ็ต Watchdog Timer เพื่อป้องกันการรีเซ็ต MCU
    delay(1000);

    // เปลี่ยนสถานะรีเลย์สำหรับรอบถัดไป
    // วนลูปรีเลย์จาก 0 ถึง 4
    // เมื่อถึง 4 จะกลับไปที่ 0
    relayStep++;
    if (relayStep > 4)
    {
        relayStep = 0;
    }
}
