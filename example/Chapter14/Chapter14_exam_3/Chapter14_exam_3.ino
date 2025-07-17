/***********************************************************************
 * Project      :     tenergy32gateway_lora_mqtt
 * Description  :     Receive JSON data from Sensor Board via LoRa, display
 *                    on OLED/Serial, and forward to MQTT Broker (Raspberry Pi)
 * Hardware     :     Tenergy32GateWay
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
#include <ArduinoJson.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <map>

/**************************************/
// Object variables
/**************************************/
Tenergy32GateWay mcu;               // อ็อบเจกต์ควบคุมบอร์ด Gateway
WiFiClient espClient;               // อ็อบเจกต์สำหรับเชื่อมต่อ WiFi
PubSubClient mqttClient(espClient); // อ็อบเจกต์สำหรับเชื่อมต่อ MQTT

static std::map<String, int> lastCounterMap; // เก็บ counter ล่าสุดของแต่ละ id

/**************************************/
// Constants variables
/**************************************/
// กำหนด LoRa IDs ที่อนุญาตให้รับข้อมูล
const char *lora_id_1 = "esp32hub-000001"; // เปลี่ยนเป็น ID ของบอร์ดที่ต้องการ
const char *lora_id_2 = "esp32hub-000002"; // เพิ่ม ID ได้ตามต้องการ
const char *lora_id_3 = "esp32hub-000003"; // เพิ่ม ID ได้ตามต้องการ

// กำหนด WiFi และ MQTT
#define WIFI_SSID "YOUR_WIFI_SSID" // เปลี่ยนเป็น SSID ของคุณ
#define WIFI_PASSWORD "YOUR_WIFI_PASSWORD" // เปลี่ยนเป็นรหัสผ่าน WiFi ของคุณ
#define MQTT_SERVER "YOUR_MQTT_SERVER" // IP Raspberry Pi
#define MQTT_PORT 1883
const char *mqtt_user_char = "tiny32";
const char *mqtt_pass_char = "tiny32";
const char *mqtt_topic_char = "esp32hub";

/**************************************/
/*           define function          */
/**************************************/

/**************************************/
/*           Global variable          */
/**************************************/
String unitName = "";
String firmwareVersion = "1.0"; // กำหนดเวอร์ชันเฟิร์มแวร์
char topic_publish[64];         // ตัวแปรสำหรับเก็บชื่อ topic ที่จะ publish ข้อมูล
char topic_subscribe[64];       // ตัวแปรสำหรับเก็บชื่อ topic ที่จะ subscribe

/***********************************************************************
 * FUNCTION:    getUnitNameFromMac
 * DESCRIPTION: สร้างชื่อ unitName จาก MAC Address (6 ตัวหลัง)
 * PARAMETERS:   None
 * RETURNED:    String ชื่อบอร์ด esp32gw-xxxxxx
 ***********************************************************************/
String getUnitNameFromMac()
{
    uint8_t mac[6];                      // สร้าง array สำหรับเก็บ MAC Address
    esp_read_mac(mac, ESP_MAC_WIFI_STA); // อ่าน MAC Address ของ WiFi STA
    char macStr[7];
    snprintf(macStr, sizeof(macStr), "%02X%02X%02X", mac[3], mac[4], mac[5]); // แปลง 3 ไบต์ท้ายเป็น string
    return "esp32gw-" + String(macStr);                                       // คืนค่าเป็นชื่อบอร์ด
}

/***********************************************************************
 * FUNCTION:    header_print
 * DESCRIPTION: แสดงข้อมูลโปรเจคบน Serial Monitor
 * PARAMETERS:   None
 * RETURNED:    None
 ***********************************************************************/
void header_print(void)
{
    Serial.printf("\r\n***********************************************************************\r\n");
    Serial.printf("* Project      :     tenergy32gateway_lora_mqtt\r\n");
    Serial.printf("* Description  :     Receive JSON data from Sensor Board via LoRa, display on OLED/Serial, and forward to MQTT Broker (Raspberry Pi)\r\n");
    Serial.printf("* Hardware     :     Tenergy32GateWay\r\n");
    Serial.printf("* Author       :     Tenergy Innovation Co., Ltd.\r\n");
    Serial.printf("* Date         :     16/07/2025\r\n");
    Serial.printf("* Revision     :     %s\r\n", mcu._version.c_str());
    Serial.printf("* website      :     http://www.tenergyinnovation.co.th\r\n");
    Serial.printf("* Email        :     uten.boonliam@tenergyinnovation.co.th\r\n");
    Serial.printf("* TEL          :     +66 89-140-7205\r\n");
    Serial.printf("***********************************************************************/\r\n");
}

/************************************************************************
 * FUNCTION:    isAllowedId
 * DESCRIPTION: ฟังก์ชันตรวจสอบ id ว่าเป็น id ที่อนุญาตหรือไม่
 * PARAMETERS:   const char *id - id ที่ต้องการตรวจสอบ
 * RETURNED:    true ถ้า id ตรงกับที่อนุญาต, false ถ้าไม่ตรง
 ***********************************************************************/
bool isAllowedId(const char *id)
{
    // เปรียบเทียบ id ที่รับเข้ามากับ id ที่อนุญาต
    return (strcmp(id, lora_id_1) == 0) ||
           (strcmp(id, lora_id_2) == 0) ||
           (strcmp(id, lora_id_3) == 0);
}

/***********************************************************************
 * FUNCTION:    setup
 * DESCRIPTION: ฟังก์ชันเริ่มต้นสำหรับการเชื่อมต่อ WiFi และ MQTT
 * PARAMETERS:   None
 * RETURNED:    None
 ***********************************************************************/
void setup()
{
    Serial.begin(115200); // เริ่มต้น Serial Monitor ที่ baudrate 115200
    header_print();       // แสดงข้อมูลโปรเจค

    if (!mcu.begin()) // เริ่มต้นบอร์ด Gateway
    {
        Serial.println("Board initialization failed!");
        while (1)
            ; // ถ้าเริ่มต้นบอร์ดไม่สำเร็จ ให้ค้างไว้
    }

    mcu.initLCD(LCD_ADDRESS, 16, 2); // เริ่มต้นหน้าจอ OLED
    mcu.displayLCD("Tenergy32Gateway", 0, 0);
    mcu._lcd->setCursor(0, 1);                             // ตั้ง cursor ที่บรรทัดที่ 2
    mcu._lcd->printf("Version: %s", mcu._version.c_str()); // แสดงเวอร์ชันบน LCD

    // Delay เพื่อให้เห็นข้อมูลเริ่มต้น
    delay(1000);

    Serial.println("WiFi connecting...");
    mcu.displayOLED("Connecting WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    // เพิ่ม timeout สำหรับการเชื่อมต่อ WiFi (สูงสุด 15 วินาที)
    int wifiTimeout = 0;
    const int wifiTimeoutMax = 30; // 30*500ms = 15 วินาที

    while (WiFi.status() != WL_CONNECTED && wifiTimeout < wifiTimeoutMax)
    {
        delay(500);
        wifiTimeout++;
    }

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.println("WiFi connected!");
        mcu.displayOLED("WiFi connected!");
        Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
        String ipStr = WiFi.localIP().toString();
        mcu.displayOLEDLines("WiFi connected!", "IP Address:", ipStr.c_str());
    }
    else
    {
        Serial.println("WiFi connect failed!");
        mcu.displayOLED("WiFi connect failed!");
        mcu.beep(3, 200); // Beep 3 ครั้งเพื่อแจ้งเตือน
        // ค้างรอให้ผู้ใช้แก้ไขปัญหา
        while (1)
            delay(1000);
    }

    Serial.println("MQTT connecting...");
    mcu.displayOLED("Connecting MQTT...");
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT); // กำหนด MQTT Server และ Port
    while (!mqttClient.connected())
    {
        mqttClient.connect(mqtt_topic_char, mqtt_user_char, mqtt_pass_char); // พยายามเชื่อมต่อ MQTT
        mqttClient.setBufferSize(512);                                       // กำหนด buffer size สำหรับ MQTT client
        if (mqttClient.connect(mqtt_topic_char, mqtt_user_char, mqtt_pass_char))
        {
            Serial.println("MQTT connected!");
            mcu.displayOLED("MQTT connected!");
        }
        else
        {
            mcu.beep(3, 100); // Beep 3 ครั้งเพื่อแจ้งเตือน
            Serial.print("MQTT connect failed, rc=");
            Serial.println(mqttClient.state());
            mcu.displayOLED("MQTT connect failed!");
            while (1) // ค้างรอให้ผู้ใช้แก้ไขปัญหา
            {
                delay(1000);
            }
        }
        delay(500);
    }

    char _line1[22], _line2[22], _line3[22], _line4[22]; // ตัวแปรสำหรับแสดงผลบน OLED
    // สร้างชื่อ unitName จาก MAC Address
    unitName = getUnitNameFromMac();
    Serial.printf("\tunitName: %s\r\n", unitName.c_str());
    snprintf(_line1, sizeof(_line1), "unit:%s", unitName.c_str());

    //*** generate topic_public ***
    String _strtmp = String(mqtt_topic_char) + "/" + String(unitName);
    _strtmp.toCharArray(topic_publish, _strtmp.length() + 1);
    Serial.printf("\ttopic_pub %s\r\n", topic_publish);
    snprintf(_line2, sizeof(_line2), "pub:%s", topic_publish);

    //*** generate topic_subscribe ***
    _strtmp = String(mqtt_topic_char) + "/" + String(unitName) + "/control";
    _strtmp.toCharArray(topic_subscribe, _strtmp.length() + 1);
    Serial.printf("\ttopic_sub %s\r\n", topic_subscribe);
    snprintf(_line3, sizeof(_line3), "sub:%s", topic_subscribe);

    //**mqtt broker */
    snprintf(_line4, sizeof(_line4), "mqtt_broker:%s", MQTT_SERVER);
    Serial.printf("\tserver: %s\r\n", MQTT_SERVER);

    // แสดงผลบน OLED 4 บรรทัด
    mcu.displayOLEDLines(_line1, _line2, _line3, _line4);

    // Initialize watchdog timer (ตั้ง watchdog timer 10 วินาที)
    esp_task_wdt_init(10, true);
    esp_task_wdt_add(NULL);
}

/***********************************************************************
 * FUNCTION:    loop
 * DESCRIPTION: ฟังก์ชันหลักสำหรับรับข้อมูลจาก LoRa, แสดงบน OLED/Serial, และส่งไปยัง MQTT Broker
 * PARAMETERS:   None
 * RETURNED:    None
 ***********************************************************************/
void loop()
{

    uint8_t _hour;
    uint8_t _minute;
    uint8_t _second;

    // --- รับข้อมูลจาก LoRa ---
    uint8_t buffer[256]; // สร้าง buffer สำหรับรับข้อมูล LoRa (ขนาด 256 bytes)
    int received = 0;    // ตัวแปรเก็บจำนวน byte ที่รับได้

    // ถ้ามีข้อมูล LoRa เข้ามาและรับได้มากกว่า 0 byte
    if (mcu.receiveLoRa(buffer, sizeof(buffer), received) && received > 0)
    {
        int lora_rssi = LoRa.packetRssi();
        buffer[received] = '\0';
        String jsonStr = (char *)buffer;

        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, jsonStr);

        if (!error)
        {
            const char *id = doc["id"] | ""; // ประกาศครั้งเดียวตรงนี้
            int counter = doc["counter"] | 0;

            // ตรวจสอบ id ว่าได้รับอนุญาตหรือไม่
            if (!isAllowedId(id))
            {
                Serial.println("ID not allowed, ignore packet.");
                return;
            }

            // ตรวจสอบ counter ซ้ำเฉพาะแต่ละ id
            if (lastCounterMap[String(id)] == counter)
            {
                Serial.println("Duplicate counter for this id, ignore packet.");
                return;
            }
            lastCounterMap[String(id)] = counter; // อัปเดต counter ล่าสุดของ id นี้

            // แสดงเวลาในรูปแบบ HH:MM:SS
            mcu.getTime(_hour, _minute, _second);
            Serial.printf("Time: %02d:%02d:%02d\n", _hour, _minute, _second);

            // --- เตรียม JSON ใหม่สำหรับส่ง MQTT ---
            StaticJsonDocument<512> mqttDoc;
            // ข้อมูลฝั่ง Gateway
            char timeStr[9]; // HH:MM:SS + null
            snprintf(timeStr, sizeof(timeStr), "%02d:%02d:%02d", _hour, _minute, _second);
            mqttDoc["timestamp"] = timeStr;
            mqttDoc["gateway_id"] = unitName;
            mqttDoc["gateway_fw"] = firmwareVersion;
            mqttDoc["gateway_topic"] = mqtt_topic_char;
            mqttDoc["wifi_ssid"] = WIFI_SSID;
            mqttDoc["wifi_ip"] = WiFi.localIP().toString();
            mqttDoc["wifi_rssi"] = WiFi.RSSI();

            // ข้อมูลฝั่ง Client (LoRa)
            mqttDoc["client_id"] = doc["id"] | "";
            mqttDoc["client_counter"] = doc["counter"] | 0;
            mqttDoc["client_fw"] = doc["fw"] | "";
            mqttDoc["client_topic"] = doc["topic"] | "";
            mqttDoc["client_rssi"] = lora_rssi;

            // ข้อมูลเซนเซอร์ param_1 ถึง param_10
            for (int i = 1; i <= 10; ++i)
            {
                String key = "param_" + String(i);
                mqttDoc[key] = doc[key] | 0.0;
            }

            // แสดงข้อมูลทาง LCD
            mcu._lcd->clear();         // ล้างหน้าจอ LCD
            mcu._lcd->setCursor(0, 0); // ตั้ง cursor ที่บรรทัดที่ 1
            mcu._lcd->printf("%s", doc["id"] | "");
            mcu._lcd->setCursor(0, 1); // ตั้ง cursor ที่บรรทัดที่ 2
            mcu._lcd->printf("RSSI:%ddBm", lora_rssi);

            // แปลงเป็น JSON string เพื่อส่ง MQTT
            char mqttBuffer[512];
            size_t mqttLen = serializeJson(mqttDoc, mqttBuffer, sizeof(mqttBuffer));
            if (mqttClient.connected())
            {
                mqttClient.publish(topic_publish, mqttBuffer, mqttLen);
            }

            // แสดงข้อมูลที่รับจาก LoRa บน Serial Monitor
            Serial.println("=== LoRa JSON Received ===");
            Serial.printf("id: %s\n", doc["id"] | "");
            Serial.printf("fw: %s\n", doc["fw"] | "");
            Serial.printf("topic: %s\n", doc["topic"] | "");
            Serial.printf("counter: %d\n", doc["counter"] | 0);

            // วนลูปแสดงค่าพารามิเตอร์ param_1 ถึง param_10
            for (int i = 1; i <= 10; ++i)
            {
                String key = "param_" + String(i);
                Serial.printf("%s: %.2f\n", key.c_str(), doc[key] | 0.0);
            }
            Serial.printf("RSSI: %d dBm\n", lora_rssi);

            id = doc["id"] | ""; // อ่านค่า id จาก JSON
            if (isAllowedId(id)) // ตรวจสอบว่า id นี้ได้รับอนุญาตหรือไม่
            {
                Serial.printf("Data size: %d bytes\n", received);

                // เตรียมข้อความสำหรับแสดงบน OLED
                char oledLine1[32], oledLine2[32], oledLine3[32], oledLine4[32];
                snprintf(oledLine1, sizeof(oledLine1), "id:%s", id);
                snprintf(oledLine2, sizeof(oledLine2), "rssi:%ddB", lora_rssi);
                snprintf(oledLine3, sizeof(oledLine3), "Size:%d Bytes", received);
                snprintf(oledLine4, sizeof(oledLine4), "cnt:%d", doc["counter"] | 0);

                // แสดงข้อมูลบน OLED 4 บรรทัด
                mcu.displayOLEDLines(oledLine1, oledLine2, oledLine3, oledLine4);
            }
            // ถ้า id ไม่ตรงกับที่อนุญาต จะไม่แสดงบน OLED และไม่ส่ง MQTT
        }
        else
        {
            Serial.println("JSON parse failed!");
            mcu.displayOLED("JSON parse failed!");
        }
    }

    mqttClient.loop(); // ให้ MQTT client ทำงาน (เช่น รับ/ส่งข้อมูล)

    esp_task_wdt_reset(); // รีเซ็ต watchdog timer ป้องกัน MCU รีเซ็ต
    delay(100);           // หน่วงเวลา 100 ms

    if (mcu.readSW1()) // ถ้า SW1 ถูกกด
    {
        mcu.beep(1, 100);                                    // Beep 1 ครั้ง
        char _line1[22], _line2[22], _line3[22], _line4[22]; // ตัวแปรสำหรับแสดงผลบน OLED

        snprintf(_line1, sizeof(_line1), "unit:%s", unitName.c_str());
        snprintf(_line2, sizeof(_line2), "pub:%s", topic_publish);
        snprintf(_line3, sizeof(_line3), "sub:%s", topic_subscribe);
        snprintf(_line4, sizeof(_line4), "server:%s", MQTT_SERVER);

        Serial.printf("\tunitName: %s\r\n", unitName.c_str());
        Serial.printf("\ttopic_pub: %s\r\n", topic_publish);
        Serial.printf("\ttopic_sub: %s\r\n", topic_subscribe);
        Serial.printf("\tmqtt_broker: %s\r\n", MQTT_SERVER);

        // แสดงผลบน OLED 4 บรรทัด
        mcu.displayOLEDLines(_line1, _line2, _line3, _line4);
    }
}