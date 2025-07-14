
/***********************************************************************
 * Project      :     Tenergy32Gateway RTC DateTime setting time Demo
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
    Serial.printf("\r\n*******************************************\r\n");
    Serial.printf("* Project      :     Tenergy32Gateway RTC DateTime setting time Demo\r\n");
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
    // --- โหมดตั้งค่าวันที่และเวลา RTC ด้วยปุ่ม SW1/SW2 ---
    static bool settingMode = false;                                       // true = กำลังตั้งค่า, false = โหมดปกติ
    static uint8_t setStep = 0;                                            // 0=ปี, 1=เดือน, 2=วัน, 3=ชั่วโมง, 4=นาที, 5=ยืนยัน
    static uint16_t setYear = 2025;                                        // ตัวแปรเก็บค่าปีที่ตั้ง
    static uint8_t setMonth = 7, setDay = 14, setHour = 12, setMinute = 0; // ตัวแปรเก็บค่าเดือน วัน ชั่วโมง นาที
    static unsigned long lastBtnTime = 0;                                  // สำหรับ debounce ปุ่ม
    static bool lastSW1 = false, lastSW2 = false;                          // เก็บสถานะปุ่มรอบก่อน

    // อ่านสถานะปุ่ม SW1 และ SW2
    bool sw1 = mcu.readSW1();
    bool sw2 = mcu.readSW2();

    // --- ปรับปรุง: รองรับการกด SW1 ค้างเพื่อเพิ่มค่าอัตโนมัติ ---
    static unsigned long sw1HoldStart = 0; // เวลาที่เริ่มกด SW1
    static unsigned long lastAutoInc = 0;  // เวลาที่เพิ่มค่าอัตโนมัติครั้งล่าสุด
    bool sw1Pressed = false;
    bool sw2Pressed = false;

    // ตรวจจับการกด SW1/SW2 แบบครั้งเดียว (edge)
    if (sw1 && !lastSW1 && millis() - lastBtnTime > 200) {
        sw1Pressed = true;
        sw1HoldStart = millis();
        lastAutoInc = millis();
        lastBtnTime = millis();
    }
    if (sw2 && !lastSW2 && millis() - lastBtnTime > 200) {
        sw2Pressed = true;
        lastBtnTime = millis();
    }

    // ตรวจจับการกด SW1 ค้าง (hold)
    if (sw1 && lastSW1 && settingMode) {
        // ถ้ากดค้างเกิน 600ms ให้เริ่ม auto increment
        if (millis() - sw1HoldStart > 600 && millis() - lastAutoInc > 120) {
            sw1Pressed = true;
            lastAutoInc = millis();
        }
    }
    if (!sw1) sw1HoldStart = 0; // reset เมื่อปล่อยปุ่ม

    lastSW1 = sw1; // เก็บสถานะปุ่มรอบล่าสุด
    lastSW2 = sw2;

    // ถ้ายังไม่อยู่ในโหมดตั้งค่า และมีการกด SW1 ให้เข้าสู่โหมดตั้งค่า
    if (!settingMode && sw1Pressed)
    {
        mcu.beep(1, 100);   // ส่งเสียง beep เพื่อยืนยัน
        settingMode = true; // เข้าสู่โหมดตั้งค่า
        setStep = 0;        // เริ่มที่การตั้งค่าปี
        // อ่านค่าเดิมจาก RTC มาเป็นค่าเริ่มต้น
        uint16_t y;
        uint8_t m, d, h, min, s;
        mcu.getDateTime(y, m, d, h, min, s);
        setYear = y;
        setMonth = m;
        setDay = d;
        setHour = h;
        setMinute = min;
    }

    // ถ้าอยู่ในโหมดตั้งค่า
    if (settingMode)
    {
        // ถ้ามีการกด SW1 ให้เพิ่มค่าตาม step ที่กำลังตั้ง
        if (sw1Pressed)
        {
            switch (setStep)
            {
            case 0:
                setYear++;
                if (setYear > 2099)
                    setYear = 2000;
                mcu.beep(1, 100);
                break; // เพิ่มปี
            case 1:
                setMonth++;
                if (setMonth > 12)
                    setMonth = 1;
                mcu.beep(1, 100);
                break; // เพิ่มเดือน
            case 2:
                setDay++;
                if (setDay > 31)
                    setDay = 1;
                mcu.beep(1, 100);
                break; // เพิ่มวัน
            case 3:
                setHour = (setHour + 1) % 24;
                mcu.beep(1, 100);
                break; // เพิ่มชั่วโมง
            case 4:
                setMinute = (setMinute + 1) % 60;
                mcu.beep(1, 100);
                break; // เพิ่มนาที
            }
        }
        // ถ้ามีการกด SW2 ให้เลื่อนไป field ถัดไป
        if (sw2Pressed)
        {
            setStep++;        // ไปยัง step ถัดไป
            mcu.beep(2, 100); // ส่งเสียง beep 2 ครั้ง
            if (setStep > 5)
                setStep = 0; // วนกลับไปปี
            // เมื่อถึง step 5 ให้บันทึกค่าที่ตั้งไว้ลง RTC
            if (setStep == 5)
            {
                mcu.setDateTime(setYear, setMonth, setDay, setHour, setMinute, 0);
            }
            // ออกจากโหมดตั้งค่าเมื่อกด SW2 ที่ step 5 อีกครั้ง
            if (setStep == 0)
                settingMode = false;
        }

        // เตรียมข้อความแสดงผลบน OLED
        char line1[24], line2[24], line3[24], line4[24];
        snprintf(line1, sizeof(line1), "SET RTC DATE/TIME");                         // หัวข้อ
        snprintf(line2, sizeof(line2), "%04d/%02d/%02d", setYear, setMonth, setDay); // วันที่
        snprintf(line3, sizeof(line3), "%02d:%02d", setHour, setMinute);             // เวลา
        // ข้อความแนะนำการใช้งานแต่ละ step
        const char *stepMsg[] = {"SW1: +Year", "SW1: +Month", "SW1: +Day", "SW1: +Hour", "SW1: +Min", "SW2: Save"};
        snprintf(line4, sizeof(line4), "%s SW2:Next", stepMsg[setStep < 5 ? setStep : 4]);
        mcu.displayOLEDLines(line1, line2, line3, line4);       // แสดงบน OLED
        Serial.printf("[SET] %s %s %s\n", line2, line3, line4); // แสดงบน Serial
        esp_task_wdt_reset();                                   // รีเซ็ต watchdog
        delay(200);                                             // หน่วงเวลาเล็กน้อย
        return;                                                 // ออกจาก loop() เพื่อไม่ให้แสดงเวลาปกติ
    }

    // --- โหมดปกติ: แสดงวันที่และเวลาจาก RTC ---
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
