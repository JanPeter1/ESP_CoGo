// cyd_config.h - Auto-generated configuration for CYD Link
#ifndef CYD_CONFIG_H
#define CYD_CONFIG_H

#include "secrets.h"

// Cloud / Local Hub Server URL
// Note: ESP32 requires http:// (or https:// with WiFiClientSecure)
const char* HUB_SERVER_URL = "https://ais-dev-zrphkgr73okcm4gn6qtkvr-188255202179.europe-west1.run.app";
const char* ROOM_ID        = "CYD-PAIR-1";
const char* DEVICE_ROLE    = "device_a"; // "device_a" or "device_b"

// Hardware Pin Definitions for ESP32-2432S028 CYD
#define CYD_TFT_BL    21
#define CYD_TOUCH_CS  33
#define CYD_TOUCH_IRQ 36

// Onboard RGB LED (Active LOW on CYD!)
#define CYD_LED_RED   4
#define CYD_LED_GREEN 16
#define CYD_LED_BLUE  17

// Onboard Speaker / Buzzer (DAC/PWM)
#define CYD_BUZZER_PIN 26

// Onboard LDR (Light Sensor)
#define CYD_LDR_PIN   34

// Touch Screen Calibration Coordinates (Standard CYD 2.8" Resistive)
#define TS_MINX 200
#define TS_MAXX 3800
#define TS_MINY 240
#define TS_MAXY 3800

#endif
