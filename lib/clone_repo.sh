#!/bin/bash
echo "Cloning repositories..."

if ! command -v git &> /dev/null
then
    echo "git could not be found. Please install git to proceed."
    exit 1
fi

TARGET_DIR="./"

# Check if the target directory exists, if not create it
if [ ! -d "$TARGET_DIR" ]; then
    mkdir -p "$TARGET_DIR"
fi

cd "$TARGET_DIR" || exit

# Define the repository URLs (ไม่ซ้ำกัน)
REPO_URLS=(
    "https://github.com/tenergyinnovation/tenergy32gateway.git"
    "https://github.com/adafruit/Adafruit_SSD1306.git"
    "https://github.com/sandeepmistry/arduino-LoRa.git"
    "https://github.com/adafruit/Adafruit_BusIO.git"
    "https://github.com/adafruit/Adafruit_Sensor.git"
    "https://github.com/adafruit/RTClib.git"
    "https://github.com/johnrickman/LiquidCrystal_I2C.git"
    "https://github.com/adafruit/Adafruit-GFX-Library.git"
    "https://github.com/PaulStoffregen/Ethernet.git"
    "https://github.com/bblanchon/ArduinoJson.git"
    "https://github.com/tenergyinnovation/tiny32_v3.git"
    "https://github.com/knolleary/pubsubclient.git"
)

for REPO_URL in "${REPO_URLS[@]}"; do
    git clone "$REPO_URL"
done

echo "Repositories cloned successfully."
ls -l