/**
 * CYD Link - ESP32 Cheap Yellow Display Hub & 2-Player Game Firmware
 * Hardware: ESP32-2432S028 (ILI9341 320x240 TFT + XPT2046 Touch)
 * 
 * Features:
 * - Two displays paired via Cloud Hub Server (CYD-PAIR-1)
 * - Send & Receive Messages & Emojis
 * - Play 4 Wins (Connect 4) and Schiffe versenken (Battleship)
 * - Receive and display Companion App Photos & Sketches
 * - Onboard RGB LED & Buzzer feedback
 */

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include "cyd_config.h"

// Hardware Drivers
TFT_eSPI tft = TFT_eSPI();
SPIClass mySPI = SPIClass(VSPI);
XPT2046_Touchscreen ts(CYD_TOUCH_CS, CYD_TOUCH_IRQ);

// State Variables
unsigned long lastPollTime = 0;
const unsigned long POLL_INTERVAL_MS = 1500; // Fast 1.5s refresh
unsigned long lastSinceTimestamp = 0;

enum DisplayMode {
  SCREEN_DASHBOARD,
  SCREEN_CONNECT4,
  SCREEN_BATTLESHIP,
  SCREEN_MESSAGES,
  SCREEN_PICTURE
};

DisplayMode currentScreen = SCREEN_DASHBOARD;
String currentTurn = "device_a";
String gameWinner = "";
int connect4Board[6][7]; // 0=empty, 1=player A, 2=player B
String latestMessageText = "Bereit zum Koppeln!";
String latestSender = "System";

void setRgbLed(bool r, bool g, bool b) {
  // CYD RGB LED is Active LOW
  digitalWrite(CYD_LED_RED,   r ? LOW : HIGH);
  digitalWrite(CYD_LED_GREEN, g ? LOW : HIGH);
  digitalWrite(CYD_LED_BLUE,  b ? LOW : HIGH);
}

void playBeep(int freq, int durationMs) {
  tone(CYD_BUZZER_PIN, freq, durationMs);
}

void setup() {
  Serial.begin(115200);
  delay(500);
  Serial.println("\n=== Starting CYD Link Firmware ===");

  // Initialize LEDs
  pinMode(CYD_LED_RED, OUTPUT);
  pinMode(CYD_LED_GREEN, OUTPUT);
  pinMode(CYD_LED_BLUE, OUTPUT);
  setRgbLed(true, true, false); // Yellow during boot

  // Initialize Buzzer
  pinMode(CYD_BUZZER_PIN, OUTPUT);
  playBeep(1200, 50);

  // Initialize Display (Landscape 320x240)
  tft.init();
  tft.setRotation(1); // Landscape with USB port at bottom
  tft.fillScreen(TFT_BLACK);
  
  // Turn on Backlight
  pinMode(CYD_TFT_BL, OUTPUT);
  digitalWrite(CYD_TFT_BL, HIGH);

  // Initialize Touch
  mySPI.begin(14, 12, 13, 33); // SCLK, MISO, MOSI, SS
  ts.begin(mySPI);
  ts.setRotation(1);

  // Splash Screen
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.setTextSize(2);
  tft.drawCentreString("CYD LINK", 160, 40, 4);
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawCentreString("ESP32-2432S028 IoT Hub", 160, 90, 2);
  tft.drawCentreString("Connecting to WiFi: " + String(WIFI_SSID) + "...", 160, 130, 2);

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  int wifiAttempts = 0;
  while (WiFi.status() != WL_CONNECTED && wifiAttempts < 30) {
    delay(500);
    Serial.print(".");
    wifiAttempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected! IP: " + WiFi.localIP().toString());
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawCentreString("Connected! IP: " + WiFi.localIP().toString(), 160, 160, 2);
    setRgbLed(false, true, false); // Green
    playBeep(2000, 100);
  } else {
    Serial.println("\nWiFi connection failed! Offline demo mode.");
    tft.setTextColor(TFT_RED, TFT_BLACK);
    tft.drawCentreString("WiFi Failed. Running Demo...", 160, 160, 2);
    setRgbLed(true, false, false); // Red
  }

  delay(1500);
  drawMainUI();
}

void drawMainUI() {
  tft.fillScreen(TFT_BLACK);

  // Header Bar (Yellow/Dark Amber Maker Aesthetic)
  tft.fillRect(0, 0, 320, 28, 0x2124); // Dark slate
  tft.drawFastHLine(0, 28, 320, 0xFD20); // Amber divider line

  tft.setTextColor(TFT_YELLOW, 0x2124);
  tft.setTextSize(1);
  tft.drawString("CYD LINK: " + String(ROOM_ID), 8, 8, 2);

  tft.setTextColor(TFT_CYAN, 0x2124);
  String roleStr = (String(DEVICE_ROLE) == "device_a") ? "DEV #1" : "DEV #2";
  tft.drawRightString(roleStr + " [ONLINE]", 312, 8, 2);

  // Bottom Navigation Buttons (Large touch targets for 2.8" screen)
  drawNavButtons();

  // Screen Content
  if (currentScreen == SCREEN_DASHBOARD) {
    drawDashboard();
  } else if (currentScreen == SCREEN_CONNECT4) {
    drawConnect4();
  } else if (currentScreen == SCREEN_BATTLESHIP) {
    drawBattleship();
  } else if (currentScreen == SCREEN_MESSAGES) {
    drawMessages();
  }
}

void drawNavButtons() {
  int y = 202;
  int h = 36;
  int w = 78;

  // 4 Tabs: Home, 4-Wins, Schiffe, Chat
  tft.fillRoundRect(2,   y, w, h, 4, currentScreen == SCREEN_DASHBOARD  ? 0xFD20 : 0x2945);
  tft.fillRoundRect(82,  y, w, h, 4, currentScreen == SCREEN_CONNECT4   ? 0xFD20 : 0x2945);
  tft.fillRoundRect(162, y, w, h, 4, currentScreen == SCREEN_BATTLESHIP ? 0xFD20 : 0x2945);
  tft.fillRoundRect(242, y, w, h, 4, currentScreen == SCREEN_MESSAGES   ? 0xFD20 : 0x2945);

  tft.setTextColor(currentScreen == SCREEN_DASHBOARD ? TFT_BLACK : TFT_WHITE);
  tft.drawCentreString("HOME", 41, y + 10, 2);

  tft.setTextColor(currentScreen == SCREEN_CONNECT4 ? TFT_BLACK : TFT_WHITE);
  tft.drawCentreString("4-WINS", 121, y + 10, 2);

  tft.setTextColor(currentScreen == SCREEN_BATTLESHIP ? TFT_BLACK : TFT_WHITE);
  tft.drawCentreString("SCHIFFE", 201, y + 10, 2);

  tft.setTextColor(currentScreen == SCREEN_MESSAGES ? TFT_BLACK : TFT_WHITE);
  tft.drawCentreString("CHAT", 281, y + 10, 2);
}

void drawDashboard() {
  tft.fillRect(0, 30, 320, 170, TFT_BLACK);

  // Status Cards
  tft.drawRoundRect(10, 36, 145, 75, 4, 0x4A69);
  tft.setTextColor(0x8410, TFT_BLACK);
  tft.drawString("GEKOPPELTER PARTNER", 18, 42, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("CYD Display #2", 18, 56, 2);
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.drawString("* Synchronisiert", 18, 80, 2);

  tft.drawRoundRect(165, 36, 145, 75, 4, 0x4A69);
  tft.setTextColor(0x8410, TFT_BLACK);
  tft.drawString("AKTIVES SPIEL", 173, 42, 1);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("4 Gewinnt", 173, 56, 2);
  tft.setTextColor(TFT_CYAN, TFT_BLACK);
  tft.drawString(currentTurn == DEVICE_ROLE ? "> DU BIST DRAN!" : "Warte auf Zug...", 173, 80, 2);

  // Latest Message Strip
  tft.drawRoundRect(10, 120, 300, 72, 4, 0x4A69);
  tft.setTextColor(0xFD20, TFT_BLACK);
  tft.drawString("LETZTE NACHRICHT (" + latestSender + "):", 18, 126, 1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(latestMessageText, 18, 145, 2);

  // Quick Emoji Touch Buttons
  tft.fillRoundRect(18,  166, 48, 22, 3, 0x2124);
  tft.drawString(":)", 36, 170, 2);
  tft.fillRoundRect(72,  166, 48, 22, 3, 0x2124);
  tft.drawString("<3", 90, 170, 2);
  tft.fillRoundRect(126, 166, 48, 22, 3, 0x2124);
  tft.drawString("OK!", 144, 170, 2);
  tft.fillRoundRect(180, 166, 60, 22, 3, 0x2124);
  tft.drawString("SPIEL?", 198, 170, 2);
}

void drawConnect4() {
  tft.fillRect(0, 30, 320, 170, 0x0010); // Deep navy background

  // Game Info Header
  tft.setTextColor(TFT_WHITE, 0x0010);
  tft.drawString("4 GEWINNT (VIER GEWINNT)", 12, 34, 2);

  bool isMyTurn = (currentTurn == DEVICE_ROLE);
  if (gameWinner != "") {
    tft.setTextColor(gameWinner == DEVICE_ROLE ? TFT_GREEN : TFT_RED, 0x0010);
    tft.drawRightString(gameWinner == DEVICE_ROLE ? "GEWONNEN!" : "VERLOREN!", 308, 34, 2);
  } else {
    tft.setTextColor(isMyTurn ? TFT_YELLOW : 0x8410, 0x0010);
    tft.drawRightString(isMyTurn ? "> DEIN ZUG (Tippe Spalte)" : "Gegner zieht...", 308, 34, 2);
  }

  // Draw 7x6 Connect 4 Grid (centered)
  int startX = 40;
  int startY = 56;
  int cellSize = 22;

  // Blue grid plate
  tft.fillRoundRect(startX - 6, startY - 4, 7 * cellSize + 12, 6 * cellSize + 8, 6, 0x0219);

  for (int r = 0; r < 6; r++) {
    for (int c = 0; c < 7; c++) {
      int px = startX + c * cellSize + cellSize / 2;
      int py = startY + r * cellSize + cellSize / 2;
      int val = connect4Board[r][c];

      uint16_t color = 0x0008; // Empty hole
      if (val == 1) color = TFT_RED;    // Player A
      if (val == 2) color = TFT_YELLOW; // Player B

      tft.fillCircle(px, py, 8, color);
      tft.drawCircle(px, py, 8, 0x0114);
    }
  }

  // Touch column guide indicators on top of grid
  for (int c = 0; c < 7; c++) {
    int px = startX + c * cellSize + cellSize / 2;
    tft.drawTriangle(px - 3, startY - 10, px + 3, startY - 10, px, startY - 6, TFT_YELLOW);
  }

  // Reset Game Button
  tft.fillRoundRect(240, 90, 72, 34, 4, 0x4A69);
  tft.setTextColor(TFT_WHITE, 0x4A69);
  tft.drawCentreString("NEUES", 276, 94, 1);
  tft.drawCentreString("SPIEL", 276, 108, 1);
}

void drawBattleship() {
  tft.fillRect(0, 30, 320, 170, 0x0008);
  tft.setTextColor(TFT_CYAN, 0x0008);
  tft.drawString("SCHIFFE VERSENKEN (RADAR)", 12, 34, 2);

  // 7x7 Target Grid
  int startX = 20;
  int startY = 56;
  int cellSize = 19;

  for (int y = 0; y < 7; y++) {
    for (int x = 0; x < 7; x++) {
      int px = startX + x * cellSize;
      int py = startY + y * cellSize;
      tft.drawRect(px, py, cellSize, cellSize, 0x2410);
      tft.fillRect(px + 1, py + 1, cellSize - 2, cellSize - 2, 0x0821);
    }
  }

  // Legend / Instructions
  tft.setTextColor(TFT_WHITE, 0x0008);
  tft.drawString("Ziel waehlen:", 170, 60, 2);
  tft.setTextColor(0x8410, 0x0008);
  tft.drawString("- Tippe auf Raster", 170, 80, 1);
  tft.drawString("- Rot = Treffer", 170, 96, 1);
  tft.drawString("- Blau = Wasser", 170, 112, 1);

  bool isMyTurn = (currentTurn == DEVICE_ROLE);
  tft.setTextColor(isMyTurn ? TFT_GREEN : TFT_ORANGE, 0x0008);
  tft.drawString(isMyTurn ? "FEUER FREI!" : "Gegner zielt...", 170, 136, 2);
}

void drawMessages() {
  tft.fillRect(0, 30, 320, 170, TFT_BLACK);
  tft.setTextColor(TFT_YELLOW, TFT_BLACK);
  tft.drawString("CYD MESSENGER", 12, 34, 2);

  // Received message bubble
  tft.fillRoundRect(12, 58, 296, 60, 6, 0x2124);
  tft.setTextColor(0xFD20, 0x2124);
  tft.drawString("VON: " + latestSender, 22, 64, 1);
  tft.setTextColor(TFT_WHITE, 0x2124);
  tft.drawString(latestMessageText, 22, 80, 2);

  // Quick reply pads
  tft.drawString("Schnellantwort senden:", 12, 126, 1);
  
  tft.fillRoundRect(12,  144, 68, 42, 4, 0xFD20);
  tft.setTextColor(TFT_BLACK, 0xFD20);
  tft.drawCentreString("JA! :-)", 46, 156, 2);

  tft.fillRoundRect(88,  144, 68, 42, 4, 0x4A69);
  tft.setTextColor(TFT_WHITE, 0x4A69);
  tft.drawCentreString("NEIN :-(", 122, 156, 2);

  tft.fillRoundRect(164, 144, 68, 42, 4, 0x1A4F);
  tft.setTextColor(TFT_WHITE, 0x1A4F);
  tft.drawCentreString("SPIELEN", 198, 156, 2);

  tft.fillRoundRect(240, 144, 68, 42, 4, 0x6200);
  tft.setTextColor(TFT_WHITE, 0x6200);
  tft.drawCentreString("PING", 274, 156, 2);
}

void sendQuickMessage(String text, String type = "text") {
  if (WiFi.status() != WL_CONNECTED) return;

  playBeep(1600, 40);
  HTTPClient http;
  String url = String(HUB_SERVER_URL) + "/api/cyd/rooms/" + String(ROOM_ID) + "/message";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["senderRole"] = DEVICE_ROLE;
  doc["senderName"] = (String(DEVICE_ROLE) == "device_a") ? "CYD Display #1" : "CYD Display #2";
  doc["type"] = type;
  doc["content"] = text;

  String payload;
  serializeJson(doc, payload);
  http.POST(payload);
  http.end();

  // Optimistic local update
  latestMessageText = text;
  latestSender = "Ich (" + String(DEVICE_ROLE) + ")";
  if (currentScreen == SCREEN_MESSAGES || currentScreen == SCREEN_DASHBOARD) {
    drawMainUI();
  }
}

void sendConnect4Move(int col) {
  if (WiFi.status() != WL_CONNECTED) return;
  playBeep(800, 60);

  HTTPClient http;
  String url = String(HUB_SERVER_URL) + "/api/cyd/rooms/" + String(ROOM_ID) + "/game/move";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  JsonDocument doc;
  doc["gameType"] = "connect4";
  doc["player"] = DEVICE_ROLE;
  doc["col"] = col;

  String payload;
  serializeJson(doc, payload);
  int httpCode = http.POST(payload);
  if (httpCode == 200) {
    Serial.println("Move sent for col: " + String(col));
  }
  http.end();
}

void pollServer() {
  if (WiFi.status() != WL_CONNECTED) return;

  HTTPClient http;
  String url = String(HUB_SERVER_URL) + "/api/cyd/poll?room=" + String(ROOM_ID) +
               "&device=" + String(DEVICE_ROLE) +
               "&since=" + String(lastSinceTimestamp);

  http.begin(url);
  int httpCode = http.GET();

  if (httpCode == 200) {
    String payload = http.getString();
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (!err) {
      lastSinceTimestamp = doc["serverTime"] | lastSinceTimestamp;
      String nextTurn = doc["connect4"]["turn"].as<String>();
      String winner = doc["connect4"]["winner"].as<String>();

      bool stateChanged = false;

      // Update Connect 4 board array
      JsonArray rows = doc["connect4"]["board"].as<JsonArray>();
      if (!rows.isNull()) {
        for (int r = 0; r < 6; r++) {
          JsonArray colArr = rows[r].as<JsonArray>();
          for (int c = 0; c < 7; c++) {
            String val = colArr[c].as<String>();
            int intVal = (val == "A") ? 1 : (val == "B" ? 2 : 0);
            if (connect4Board[r][c] != intVal) {
              connect4Board[r][c] = intVal;
              stateChanged = true;
            }
          }
        }
      }

      if (currentTurn != nextTurn || gameWinner != winner) {
        currentTurn = nextTurn;
        gameWinner = winner;
        stateChanged = true;
      }

      // Check for incoming new messages
      JsonArray newMsgs = doc["newMessages"].as<JsonArray>();
      if (!newMsgs.isNull() && newMsgs.size() > 0) {
        JsonObject lastMsg = newMsgs[newMsgs.size() - 1];
        String sRole = lastMsg["senderRole"].as<String>();
        if (sRole != DEVICE_ROLE) {
          latestMessageText = lastMsg["content"].as<String>();
          latestSender = lastMsg["senderName"].as<String>();
          // Flash Blue LED & Chime on message
          setRgbLed(false, false, true);
          playBeep(1200, 80);
          delay(80);
          playBeep(1800, 120);
          delay(200);
          setRgbLed(false, true, false);
          stateChanged = true;
        }
      }

      if (stateChanged) {
        drawMainUI();
      }
    }
  }
  http.end();
}

void handleTouch(int rawX, int rawY) {
  // Convert resistive raw coordinates to 320x240 screen pixels
  int screenX = map(rawX, TS_MINX, TS_MAXX, 0, 320);
  int screenY = map(rawY, TS_MINY, TS_MAXY, 0, 240);
  screenX = constrain(screenX, 0, 320);
  screenY = constrain(screenY, 0, 240);

  playBeep(2400, 20);

  // Bottom Nav Bar Touch Check (y >= 200)
  if (screenY >= 200) {
    if (screenX < 80) {
      currentScreen = SCREEN_DASHBOARD;
    } else if (screenX < 160) {
      currentScreen = SCREEN_CONNECT4;
    } else if (screenX < 240) {
      currentScreen = SCREEN_BATTLESHIP;
    } else {
      currentScreen = SCREEN_MESSAGES;
    }
    drawMainUI();
    delay(250); // Debounce
    return;
  }

  // Screen-specific touch handlers
  if (currentScreen == SCREEN_CONNECT4) {
    // Touch on Connect 4 column (startX = 40, cellSize = 22, 7 cols)
    if (screenY >= 45 && screenY <= 190 && screenX >= 40 && screenX <= 40 + 7 * 22) {
      int col = (screenX - 40) / 22;
      col = constrain(col, 0, 6);
      sendConnect4Move(col);
      delay(300);
    }
  } else if (currentScreen == SCREEN_MESSAGES) {
    // Touch on Quick Reply buttons (y 144..186)
    if (screenY >= 144 && screenY <= 186) {
      if (screenX >= 12 && screenX <= 80) {
        sendQuickMessage("JA! :-)");
      } else if (screenX >= 88 && screenX <= 156) {
        sendQuickMessage("NEIN :-(");
      } else if (screenX >= 164 && screenX <= 232) {
        sendQuickMessage("Lass uns spielen!");
      } else if (screenX >= 240 && screenX <= 308) {
        sendQuickMessage("PING! Hallo?");
      }
      delay(300);
    }
  } else if (currentScreen == SCREEN_DASHBOARD) {
    if (screenY >= 160 && screenY <= 192) {
      if (screenX >= 18 && screenX <= 66) sendQuickMessage(":-)");
      else if (screenX >= 72 && screenX <= 120) sendQuickMessage("<3");
      else if (screenX >= 126 && screenX <= 174) sendQuickMessage("OK!");
      else if (screenX >= 180 && screenX <= 240) sendQuickMessage("Lust auf Spiel?");
      delay(300);
    }
  }
}

void loop() {
  // Check touch controller
  if (ts.touched()) {
    TS_Point p = ts.getPoint();
    handleTouch(p.x, p.y);
  }

  // Periodic polling for updates
  if (millis() - lastPollTime > POLL_INTERVAL_MS) {
    lastPollTime = millis();
    pollServer();
  }

  delay(20);
}
