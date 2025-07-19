/***********************************************************************
 * Project      :     smartbuilding360gateway_rfid_access
 * Description  :     โปรแกรมตัวอย่างสำหรับควบคุมการเข้า-ออกอาคารอัจฉริยะ (Smart Building Access Control)
 *                    ด้วย ESP32 และโมดูล RFID RC522 Serial Port Reader 13.56MHz (UART)
 *                    ระบบจะตรวจสอบ UID ของบัตรที่อ่านได้กับ UID ที่อนุญาตในซอร์สโค้ด
 *                    หากตรงกันจะสั่งเปิด Relay เพื่อควบคุมอุปกรณ์ไฟฟ้าหรือประตู
 * Hardware     :     Tenergy32GateWay + RFID RC522 Serial Port Reader 13.56MHz
 * Author       :     Tenergy Innovation Co., Ltd.
 * Date         :     16/07/2025
 * Revision     :     1.0
 * Rev1.0       :     Original
 * website      :     http://www.tenergyinnovation.co.th
 * Email        :     uten.boonliam@tenergyinnovation.co.th
 * TEL          :     +66 89-140-7205
 ***********************************************************************/
#include <Arduino.h>
#include <tenergy32gateway.h>
#include <esp_task_wdt.h>

/**************************************/
// Object variables
/**************************************/
Tenergy32GateWay mcu; // อ็อบเจกต์ควบคุมบอร์ด Gateway

/**************************************/
// Constants variables
/**************************************/
// กำหนดขา UART2 สำหรับเชื่อมต่อกับ RFID Reader
#define RXD2 27 // ESP32 RX (รับข้อมูลจาก RFID)
#define TXD2 26 // ESP32 TX (ส่งข้อมูลไป RFID)

HardwareSerial RFIDSerial(2); // ใช้ UART2 สำหรับติดต่อกับโมดูล RFID

// กำหนด UID ที่อนุญาตให้เปิด Relay (ตัวอย่าง: 4 bytes)
const byte allowedUID[][4] = {
    {0x32, 0xB9, 0xA0, 0xBB}, // ตัวอย่าง UID 1
    // เพิ่ม UID อื่นๆ ตามต้องการ
};
const int allowedUIDCount = sizeof(allowedUID) / sizeof(allowedUID[0]);

#define RELAY_PIN 25 // กำหนดขา Relay (เปลี่ยนตามวงจรจริง)

/**************************************/
/*           define function          */
/**************************************/
void requestCard();                              // ส่งคำสั่งค้นหาการ์ด (PcdRequest)
void rfidReset();                                // รีเซ็ตโมดูล RFID (PcdReset)
void rfidAntennaOn();                            // เปิดเสาอากาศ (PcdAntennaOn)
void rfidRequest();                              // ค้นหาการ์ด (PcdRequest)
void rfidAnticoll();                             // ขอ UID (PcdAnticoll)
void rfidSelect(const byte *uid, size_t uidLen); // เลือกการ์ด (PcdSelect)
bool isAllowedUID(const byte *uid, int uidLen);  // ตรวจสอบว่า UID ตรงกับที่อนุญาตหรือไม่

void header_print(void);     // แสดง header ข้อมูลโปรเจค
String getUnitNameFromMac(); // สร้างชื่อ unitName จาก MAC Address

/**************************************/
/*           Global variable          */
/**************************************/
String unitName = "";
String firmwareVersion = "1.0"; // กำหนดเวอร์ชันเฟิร์มแวร์

/***********************************************************************
 * FUNCTION:    getUnitNameFromMac
 * DESCRIPTION: สร้างชื่อ unitName จาก MAC Address (6 ตัวหลัง)
 * RETURNED:    String ชื่อบอร์ด esp32gw-xxxxxx
 ***********************************************************************/
String getUnitNameFromMac()
{
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    char macStr[7];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X", mac[3], mac[4], mac[5]);
    return "esp32gw-" + String(macStr);
}

/***********************************************************************
 * FUNCTION:    header_print
 * DESCRIPTION: แสดงข้อมูลโปรเจคบน Serial Monitor
 ***********************************************************************/
void header_print(void)
{
    Serial.printf("\r\n***********************************************************************\r\n");
    Serial.printf("* Project      :     smartbuilding360gateway_rfid_access\r\n");
    Serial.printf("* Description  :     Smart Building Access Control with RFID \r\n");
    Serial.printf("* Hardware     :     Tenergy32GateWay +  RFID RC522 Serial Port Reader 13.56MHz\r\n");
    Serial.printf("* Author       :     Tenergy Innovation Co., Ltd.\r\n");
    Serial.printf("* Date         :     19/07/2025\r\n");
    Serial.printf("* Revision     :     %s\r\n", mcu._version.c_str());
    Serial.printf("* website      :     http://www.tenergyinnovation.co.th\r\n");
    Serial.printf("* Email        :     uten.boonliam@tenergyinnovation.co.th\r\n");
    Serial.printf("* TEL          :     +66 89-140-7205\r\n");
    Serial.printf("***********************************************************************/\r\n");
}

/***********************************************************************
 * FUNCTION:    setup
 * DESCRIPTION: ฟังก์ชันเริ่มต้นระบบ, กำหนด Serial, LCD, OLED, RFID
 ***********************************************************************/
void setup()
{
    Serial.begin(115200); // เริ่มต้น Serial Monitor
    header_print();       // แสดงข้อมูลโปรเจค

    RFIDSerial.begin(115200, SERIAL_8N1, RXD2, TXD2); // เริ่มต้น UART2 สำหรับ RFID Reader

    if (!mcu.begin()) // เริ่มต้นบอร์ด Gateway
    {
        Serial.println("Board initialization failed!");
        while (1)
            ; // ถ้าเริ่มต้นบอร์ดไม่สำเร็จ ให้ค้างไว้
    }

    mcu.initLCD(LCD_ADDRESS, 16, 2); // เริ่มต้นหน้าจอ LCD
    mcu.displayLCD("Tenergy32Gateway", 0, 0);
    mcu._lcd->setCursor(0, 1);
    mcu._lcd->printf("Version: %s", mcu._version.c_str()); // แสดงเวอร์ชันบน LCD

    delay(1000); // หน่วงเวลาให้เห็นข้อความเริ่มต้น

    Serial.println("Waiting for RFID initialization...");
    mcu.displayOLED("Waiting for RFID initialization...");
    rfidReset(); // รีเซ็ตโมดูล RFID
    vTaskDelay(50);
    rfidAntennaOn(); // เปิดเสาอากาศ RFID
    vTaskDelay(50);
    mcu.displayOLED("RFID Ready"); // แสดงข้อความว่า RFID พร้อมใช้งาน

    pinMode(RELAY_PIN, OUTPUT);
    digitalWrite(RELAY_PIN, LOW); // ปิด Relay เริ่มต้น

    // ตั้ง watchdog timer 10 วินาที
    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);
}

/***********************************************************************
 * FUNCTION:    loop
 * DESCRIPTION: ฟังก์ชันหลักสำหรับวนอ่านบัตร RFID ด้วย state machine
 ***********************************************************************/
void loop()
{
    static unsigned long lastAction = 0; // สำหรับจับเวลาแต่ละ state
    // State machine สำหรับควบคุม flow การอ่านบัตร
    static enum { STEP_REQUEST,
                  STEP_WAIT_REQUEST,
                  STEP_ANTICOLL,
                  STEP_WAIT_ANTICOLL,
                  STEP_HALT,
                  STEP_DELAY } state = STEP_REQUEST;
    static byte uid[8]; // เก็บ UID ที่อ่านได้
    static int uidLen = 0;
    byte buffer[32]; // buffer สำหรับรับข้อมูลจาก RFID
    int index = 0;

    char _oledline1[32], _oledline2[32], _oledline3[32], _oledline4[32];

    // State machine สำหรับควบคุมลำดับการทำงาน
    switch (state)
    {
    case STEP_REQUEST:
        // ส่งคำสั่งค้นหาการ์ด (PcdRequest)
        index = 0;
        while (RFIDSerial.available())
            RFIDSerial.read(); // flush buffer
        rfidRequest();
        lastAction = millis();
        state = STEP_WAIT_REQUEST;
        break;

    case STEP_WAIT_REQUEST:
        // รอผลลัพธ์การค้นหาการ์ด
        index = 0;
        if (millis() - lastAction < 100)
            break;
        while (RFIDSerial.available() && index < sizeof(buffer))
            buffer[index++] = RFIDSerial.read();

        // แสดงข้อมูล debug
        if (index > 0)
        {
            Serial.print("[DEBUG] RFID Response: ");
            for (int i = 0; i < index; i++)
                Serial.printf("%02X ", buffer[i]);
            Serial.println();

            // แสดงสถานะบน OLED/LCD
            snprintf(_oledline1, sizeof(_oledline1), "Waiting Card");
            mcu.displayOLEDLines(_oledline1, "", "", "");
            mcu._lcd->clear();
            mcu._lcd->setCursor(0, 0);
            mcu._lcd->print("Waiting Card");
        }
        // ถ้าพบการ์ด (response pattern ถูกต้อง) ให้ไปขอ UID
        if (index >= 4 && buffer[0] == 0x7F && buffer[1] == 0x04 && buffer[2] == 0x00 && buffer[3] == 0xF7)
            state = STEP_ANTICOLL;
        else
            state = STEP_DELAY; // ไม่เจอการ์ด รอแล้ววนใหม่
        break;

    case STEP_ANTICOLL:
        // ส่งคำสั่งขอ UID (PcdAnticoll)
        index = 0;
        while (RFIDSerial.available())
            RFIDSerial.read(); // flush buffer
        rfidAnticoll();
        lastAction = millis();
        state = STEP_WAIT_ANTICOLL;
        break;

    case STEP_WAIT_ANTICOLL:
        // รอผลลัพธ์ UID
        index = 0;
        if (millis() - lastAction < 100)
            break;
        while (RFIDSerial.available() && index < sizeof(buffer))
            buffer[index++] = RFIDSerial.read();

        if (index > 0)
        {
            Serial.print("[DEBUG] UID Response: ");
            for (int i = 0; i < index; i++)
                Serial.printf("%02X ", buffer[i]);
            Serial.println();

            // ถ้าได้ UID (response pattern ถูกต้อง)
            if (index >= 6 && buffer[0] == 0x7F)
            {

                // แสดง UID บน OLED
                snprintf(_oledline1, sizeof(_oledline1), "Detect Card");
                snprintf(_oledline2, sizeof(_oledline2), "UID:%02X %02X %02X %02X",
                         buffer[1], buffer[2], buffer[3], buffer[4]);
                mcu.displayOLEDLines(_oledline1, _oledline2, "", "");

                // แสดง UID บน LCD
                mcu._lcd->clear();
                mcu._lcd->setCursor(0, 0);
                mcu._lcd->print("UID:");
                mcu._lcd->setCursor(0, 1);
                mcu._lcd->printf("%02X%02X%02X%02X", buffer[1], buffer[2], buffer[3], buffer[4]);
                vTaskDelay(300 / portTICK_PERIOD_MS); // หน่วง 1 วินาที

                // ตรวจสอบ UID (buffer[1] ถึง buffer[4])
                if (isAllowedUID(&buffer[1], 4))
                {
                    Serial.println("UID Allowed: Relay ON");
                    mcu.displayOLED("UID Allowed: Relay ON");
                    mcu._lcd->clear();
                    mcu._lcd->setCursor(0, 0);
                    mcu._lcd->print("UID Allowed");
                    mcu.beep(1, 100);
                    vTaskDelay(1000 / portTICK_PERIOD_MS); // หน่วง 1 วินาที
                    // เปิด Relay
                    mcu.relay1On(); // เปิด Relay 1

                    delay(2000);     // เปิด Relay ค้างไว้ 2 วินาที
                    mcu.relay1Off(); // ปิด Relay 1
                }
                else
                {
                    Serial.println("UID Not Allowed");
                    mcu.displayOLED("UID Not Allowed");
                    mcu._lcd->clear();
                    mcu._lcd->setCursor(0, 0);
                    mcu._lcd->print("UID Not Allowed");
                    mcu.beep(3, 100);
                }
            }
        }
        state = STEP_HALT;
        break;

    case STEP_HALT:
        // ส่งคำสั่งหยุดใช้งานการ์ด (PcdHalt)
        index = 0;
        while (RFIDSerial.available())
            RFIDSerial.read(); // flush buffer
        RFIDSerial.write(0x7F);
        RFIDSerial.write(0x0B); // Command: PcdHalt
        RFIDSerial.write(0xF7);
        lastAction = millis();
        state = STEP_DELAY;
        break;

    case STEP_DELAY:
        // หน่วงเวลาสั้นๆ ก่อนวนกลับไปค้นหาการ์ดใหม่
        index = 0;
        if (millis() - lastAction < 250)
            break;
        state = STEP_REQUEST;
        break;
    }

    // รีเซ็ต watchdog timer ทุกครั้งที่วนลูป
    esp_task_wdt_reset();
}

/***********************************************************************
 * FUNCTION:    requestCard
 * DESCRIPTION: ส่งคำสั่งค้นหาการ์ด (PcdRequest)
 ***********************************************************************/
void requestCard()
{
    RFIDSerial.write(0x7F); // Header
    RFIDSerial.write(0x03); // Command: PcdRequest
    RFIDSerial.write(0x52); // req_code: 0x52 = ค้นหาการ์ดทุกใบ
    RFIDSerial.write(0xF7); // Tail
}

/***********************************************************************
 * FUNCTION:    rfidReset
 * DESCRIPTION: ส่งคำสั่ง Reset โมดูล RFID (PcdReset)
 ***********************************************************************/
void rfidReset()
{
    RFIDSerial.write(0x7F);
    RFIDSerial.write(0x00); // Command: Reset
    RFIDSerial.write(0xF7);
}

/***********************************************************************
 * FUNCTION:    rfidAntennaOn
 * DESCRIPTION: ส่งคำสั่งเปิดเสาอากาศ (PcdAntennaOn)
 ***********************************************************************/
void rfidAntennaOn()
{
    RFIDSerial.write(0x7F);
    RFIDSerial.write(0x01); // Command: Antenna On
    RFIDSerial.write(0xF7);
}

/***********************************************************************
 * FUNCTION:    rfidRequest
 * DESCRIPTION: ส่งคำสั่งค้นหาการ์ด (PcdRequest)
 ***********************************************************************/
void rfidRequest()
{
    RFIDSerial.write(0x7F);
    RFIDSerial.write(0x03); // Command: PcdRequest
    RFIDSerial.write(0x52); // req_code: 0x52 = ค้นหาการ์ดทุกใบ
    RFIDSerial.write(0xF7);
}

/***********************************************************************
 * FUNCTION:    rfidAnticoll
 * DESCRIPTION: ส่งคำสั่งขอ UID (PcdAnticoll)
 ***********************************************************************/
void rfidAnticoll()
{
    RFIDSerial.write(0x7F);
    RFIDSerial.write(0x04); // Command: PcdAnticoll
    RFIDSerial.write(0xF7);
}

/***********************************************************************
 * FUNCTION:    rfidSelect
 * DESCRIPTION: ส่งคำสั่งเลือกการ์ด (หลังจากได้ UID) (PcdSelect)
 * PARAMETERS:  uid - ข้อมูล UID ที่ได้จากการ์ด, uidLen - ความยาว UID
 ***********************************************************************/
void rfidSelect(const byte *uid, size_t uidLen)
{
    RFIDSerial.write(0x7F);
    RFIDSerial.write(0x05); // Command: PcdSelect
    for (size_t i = 0; i < uidLen; i++)
        RFIDSerial.write(uid[i]);
    RFIDSerial.write(0xF7);
}

/***********************************************************************
 * FUNCTION:    isAllowedUID
 * DESCRIPTION: ฟังก์ชันตรวจสอบว่า UID ตรงกับที่อนุญาตหรือไม่
 * PARAMETERS:  uid - ข้อมูล UID ที่อ่านได้, uidLen - ความยาว UID
 * RETURNED:    true ถ้า UID ตรงกับที่อนุญาต, false ถ้าไม่ตรง
 ***********************************************************************/
// ฟังก์ชันตรวจสอบว่า UID ตรงกับที่อนุญาตหรือไม่
bool isAllowedUID(const byte *uid, int uidLen)
{
    for (int i = 0; i < allowedUIDCount; ++i)
    {
        bool match = true;
        for (int j = 0; j < uidLen; ++j)
        {
            if (uid[j] != allowedUID[i][j])
            {
                match = false;
                break;
            }
        }
        if (match)
            return true;
    }
    return false;
}