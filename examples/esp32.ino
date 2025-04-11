#include <WiFi.h>
#include <HardwareSerial.h>
#include <TimeLib.h>
#include <TinyGPSPlus.h>
#include <SPIFFS.h>

#include "WiFiNTPServer.h"  // Introduce NTPServer library

// Set Wi-Fi information
const char* ssid = "********";
const char* password = "********";
IPAddress local_IP(********);
IPAddress gateway(********);
IPAddress subnet(********);
IPAddress primaryDNS(********);

// WiFi Event Listener
unsigned long lastReconnectAttempt = 0;
const unsigned long reconnectInterval = 30000;  // 30 seconds
void WiFiEvent(WiFiEvent_t event) {
  if (event == WIFI_EVENT_STA_DISCONNECTED) {
    // Serial.println("[WiFi] Disconnected.");
  }
}

// TinyGPSPlus instance
TinyGPSPlus gps;

// Serial connection to GPS device
HardwareSerial ss(2);

// NTPServer instance
WiFiNTPServer ntpServer("GPS", L_NTP_STRAT_PRIMARY);

// Variable to store time information
struct tm newTime;
struct timeval tv;
bool ppsFlag = false;
unsigned long lastPPSTime = 0;

void IRAM_ATTR ppsInterrupt() {
  // PPS signal detection, set PPS signal flag
  lastPPSTime = esp_timer_get_time();  // Set lastPPSTime to the current time during initialization
  ppsFlag = true;
}

void setup() {
  Serial.begin(9600);
  ss.begin(9600, SERIAL_8N1, 16, 17, false);  // GPS 9600 / RX 16 / TX 17
  pinMode(13, INPUT);  // PPS13
  attachInterrupt(digitalPinToInterrupt(13), ppsInterrupt, RISING);
  lastPPSTime = esp_timer_get_time();  // Set lastPPSTime to the current time during initialization

  WiFi.disconnect();    // Disconnect WiFi connection
  WiFi.mode(WIFI_STA);  // Client mode
  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.setHostname("ESP_32");
  WiFi.begin(ssid, password);
  delay(1000);
  WiFi.onEvent(WiFiEvent);  // Register for WiFi events
  
  ntpServer.begin();  // Initialize NTPServer
}

void loop() {
  // After the WiFi connection is disconnected, reconnect every 30 seconds
  if (WiFi.status() != WL_CONNECTED) {
    unsigned long now = millis();
    if (now - lastReconnectAttempt >= reconnectInterval) {
      lastReconnectAttempt = now;
      Serial.println("[WiFi] Trying to reconnect...");
      WiFi.disconnect();  // To be on the safe side, clear the connection status
      WiFi.begin(ssid, password);
    }
  }
  
  // Read GPS NMEA data and pass it to TinyGPSPlus library for decoding
  while (ss.available()) {
    char c = ss.read();
    gps.encode(c);

    // Uncomment and monitor the GPS output on the serial port
    //Serial.print(c);
  }

  // If a PPS signal is received and the time since the last PPS signal is less than 1500 milliseconds, update the time
  if (ppsFlag && (esp_timer_get_time() - lastPPSTime < 1500)) {
    // Update NTPServer reference time
    struct tm newTime;
    newTime.tm_year = gps.date.year() - 1900;  // struct tm requires year offset from 1900
    newTime.tm_mon = gps.date.month() - 1;     // struct tm requires month offset from 0
    newTime.tm_mday = gps.date.day();
    newTime.tm_hour = gps.time.hour();
    newTime.tm_min = gps.time.minute();
    newTime.tm_sec = gps.time.second();
    ntpServer.setReferenceTime(newTime, esp_timer_get_time());
    
    // Update internal RTC time, which will be lost after reboot or power failure
    time_t t = mktime(&newTime);
    tv.tv_sec = t;
    tv.tv_usec = esp_timer_get_time() - lastPPSTime;
    settimeofday(&tv, NULL);

    ppsFlag = false;                     // Reset PPS signal flag
    lastPPSTime = esp_timer_get_time();  // Update timestamp
  }
  ntpServer.update();  // Respond to NTP access
}
