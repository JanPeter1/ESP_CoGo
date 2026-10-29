# ESP32 CYD (ESP32-2432S028) Firmware - CYD Link

Hardware: 2.8" Cheap Yellow Display (ILI9341 + XPT2046)
Room ID: CYD-PAIR-1
Server: https://ais-dev-zrphkgr73okcm4gn6qtkvr-188255202179.europe-west1.run.app

## PlatformIO:
1. Open this folder in VS Code with PlatformIO extension installed.
2. Copy `include/secrets.example.h` to `include/secrets.h` and set your Wi-Fi credentials.
3. Connect your ESP32 CYD via USB-C cable.
4. Run 'PlatformIO: Upload'.

## Arduino IDE:
1. Open Arduino_IDE_Sketch/CYD_Link_Sketch.ino in Arduino IDE 2.x.
2. Copy `Arduino_IDE_Sketch/secrets.example.h` to `Arduino_IDE_Sketch/secrets.h` and set your Wi-Fi credentials.
3. Install libraries: TFT_eSPI, XPT2046_Touchscreen, ArduinoJson.
4. Replace your TFT_eSPI User_Setup.h with the included User_Setup.h.
5. Board: 'ESP32 Dev Module', Flash 4MB, Upload.
