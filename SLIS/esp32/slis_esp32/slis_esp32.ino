#include <WiFi.h>
#include <HTTPClient.h>
#include "LiquidCrystal.h"

/*
 * SLIS - Smart Library Intelligence System
 * ESP32 Edge Node Firmware (Connected Architecture)
 * 
 * Hardware Configuration:
 * - 7 Seat Digital Sensors: GPIO 22, 23, 32, 33, 25, 26, 27
 * - 16x2 LiquidCrystal Display: RS=15, EN=2, D4=4, D5=16, D6=17, D7=5
 * - Dual Networking:
 *     1. Connects to Local Wi-Fi (Router/Hotspot) to sync with Flask Backend & SQLite
 *     2. Posts real-time sensor events to: http://<BACKEND_HOST>:5001/api/occupancy
 *     3. Polls http://<BACKEND_HOST>:5001/api/seats to reflect reservation states ('R') on LCD
 */

LiquidCrystal lcd(15, 2, 4, 16, 17, 5);

// Configure your Wi-Fi credentials (or phone hotspot)
const char* wifiSSID = "Your_WiFi_SSID";
const char* wifiPassword = "Your_WiFi_Password";

// Flask Backend Address (replace with your laptop's IPv4 address on the Wi-Fi network)
// Example: "http://192.168.1.100:5001" or "http://10.0.0.5:5001"
const char* backendBaseUrl = "http://172.18.157.6:5001";

const int seatPins[7] = {22, 23, 32, 33, 25, 26, 27};
bool lastSensorState[7] = {false, false, false, false, false, false, false};
char lcdSeatState[7] = {'A', 'A', 'A', 'A', 'A', 'A', 'A'};

unsigned long lastPollBackend = 0;
const unsigned long POLL_INTERVAL_MS = 2000;

void setup() {
  Serial.begin(115200);

  // Initialize seat sensor pins with internal pull-ups
  for (int i = 0; i < 7; i++) {
    pinMode(seatPins[i], INPUT_PULLUP);
    lastSensorState[i] = (digitalRead(seatPins[i]) == LOW);
  }

  // Initialize 16x2 LCD
  lcd.begin(16, 2);
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SLIS Edge Node");
  lcd.setCursor(0, 1);
  lcd.print("Connecting Wi-Fi");

  Serial.println("\n==========================================");
  Serial.println("  SLIS ESP32 Edge Node Initializing");
  Serial.println("==========================================");

  // Connect to Wi-Fi network shared with laptop
  WiFi.begin(wifiSSID, wifiPassword);
  Serial.print("Connecting to Wi-Fi: ");
  Serial.println(wifiSSID);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[+] Wi-Fi Connected!");
    Serial.print("ESP32 IP: ");
    Serial.println(WiFi.localIP());
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Wi-Fi Connected");
    lcd.setCursor(0, 1);
    lcd.print(WiFi.localIP());
  } else {
    Serial.println("\n[!] Wi-Fi not connected (check credentials in sketch).");
    Serial.println("Falling back to offline edge sensing...");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("Offline Mode");
  }

  delay(1500);
  lcd.clear();
  updateLCD();
}

void loop() {
  checkSensors();

  // Periodically sync seat states (reservations) from Flask SQLite backend
  if (WiFi.status() == WL_CONNECTED && millis() - lastPollBackend >= POLL_INTERVAL_MS) {
    lastPollBackend = millis();
    fetchBackendState();
  }

  delay(100);
}

void checkSensors() {
  bool stateChanged = false;

  for (int i = 0; i < 7; i++) {
    // LOW = occupied (circuit grounded by sensor/switch)
    bool isOccupied = (digitalRead(seatPins[i]) == LOW);

    if (isOccupied != lastSensorState[i]) {
      lastSensorState[i] = isOccupied;
      stateChanged = true;
      int seatNumber = i + 1;

      Serial.print("[SENSOR] Seat ");
      Serial.print(seatNumber);
      Serial.print(" (GPIO ");
      Serial.print(seatPins[i]);
      Serial.print(") -> ");
      Serial.println(isOccupied ? "OCCUPIED" : "VACATED");

      // Update local state for LCD
      if (isOccupied) {
        lcdSeatState[i] = 'O';
      } else {
        lcdSeatState[i] = 'A';
      }

      // Post change to Flask backend via HTTP/JSON
      if (WiFi.status() == WL_CONNECTED) {
        postOccupancyToBackend(seatNumber, isOccupied);
      }
    }
  }

  if (stateChanged) {
    updateLCD();
  }
}

void postOccupancyToBackend(int seatId, bool occupied) {
  HTTPClient http;
  String url = String(backendBaseUrl) + "/api/occupancy";
  http.begin(url);
  http.addHeader("Content-Type", "application/json");

  String payload = "{\"seat_id\":" + String(seatId) + ",\"occupied\":" + (occupied ? "true" : "false") + "}";

  int httpCode = http.POST(payload);
  if (httpCode > 0) {
    Serial.print("[HTTP POST] /api/occupancy response: ");
    Serial.println(httpCode);
  } else {
    Serial.print("[HTTP POST ERROR]: ");
    Serial.println(http.errorToString(httpCode).c_str());
  }
  http.end();
}

void fetchBackendState() {
  HTTPClient http;
  String url = String(backendBaseUrl) + "/api/seats";
  http.begin(url);

  int httpCode = http.GET();
  if (httpCode == 200) {
    String payload = http.getString();
    // Parse simple state or LCD lines from payload if provided
    int lcdIdx = payload.indexOf("\"line1\":");
    if (lcdIdx != -1) {
      int q1 = payload.indexOf("\"", lcdIdx + 8);
      int q2 = payload.indexOf("\"", q1 + 1);
      if (q1 != -1 && q2 != -1) {
        String l1 = payload.substring(q1 + 1, q2);
        int l2Idx = payload.indexOf("\"line2\":");
        if (l2Idx != -1) {
          int q3 = payload.indexOf("\"", l2Idx + 8);
          int q4 = payload.indexOf("\"", q3 + 1);
          if (q3 != -1 && q4 != -1) {
            String l2 = payload.substring(q3 + 1, q4);
            if (l1.length() >= 15) {
              lcdSeatState[0] = l1.charAt(2);
              lcdSeatState[1] = l1.charAt(6);
              lcdSeatState[2] = l1.charAt(10);
              lcdSeatState[3] = l1.charAt(14);
            }
            if (l2.length() >= 11) {
              lcdSeatState[4] = l2.charAt(2);
              lcdSeatState[5] = l2.charAt(6);
              lcdSeatState[6] = l2.charAt(10);
            }
            lcd.setCursor(0, 0);
            lcd.print(l1);
            lcd.setCursor(0, 1);
            lcd.print(l2);
            http.end();
            return;
          }
        }
      }
    }
  }
  http.end();
}

void updateLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  for (int i = 0; i < 4; i++) {
    lcd.print(String(i + 1) + ":");
    lcd.print(lcdSeatState[i]);
    if (i < 3) lcd.print(" ");
  }

  lcd.setCursor(0, 1);
  for (int i = 4; i < 7; i++) {
    lcd.print(String(i + 1) + ":");
    lcd.print(lcdSeatState[i]);
    if (i < 6) lcd.print(" ");
  }
}
