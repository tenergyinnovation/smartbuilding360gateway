@echo off
REM This script clones multiple git repositories

echo Cloning repositories...

REM Check if git is installed
git --version >nul 2>&1
if errorlevel 1 (
    echo Git is not installed. Please install Git to proceed.
    exit /b 1
)

REM Define the repository URLs (คั่นแต่ละอันด้วยช่องว่าง)
REM Define the repository URLs (คั่นแต่ละอันด้วยช่องว่าง)
REM Define the target directory (optional)
set TARGET_DIR=./

set REPO_URLS=^
https://github.com/tenergyinnovation/tenergy32gateway.git ^
https://github.com/adafruit/Adafruit_SSD1306.git ^
https://github.com/sandeepmistry/arduino-LoRa.git ^
https://github.com/adafruit/Adafruit_BusIO.git ^
https://github.com/adafruit/Adafruit_Sensor.git ^
https://github.com/adafruit/RTClib.git ^
https://github.com/johnrickman/LiquidCrystal_I2C.git ^
https://github.com/adafruit/Adafruit-GFX-Library.git ^
https://github.com/PaulStoffregen/Ethernet.git ^
https://github.com/bblanchon/ArduinoJson.git ^
https://github.com/tenergyinnovation/tiny32_v3.git ^
https://github.com/knolleary/pubsubclient.git ^

REM Create the target directory if it doesn't exist
if not exist "%TARGET_DIR%" mkdir "%TARGET_DIR%"

REM Change to the target directory
cd "%TARGET_DIR%"

REM Clone the repositories using a for loop
for %%R in (%REPO_URLS%) do (
    git clone %%R
)

REM List the contents of the target directory
echo Listing contents of the target directory:
dir /b

echo All repositories have been cloned successfully.
pause
