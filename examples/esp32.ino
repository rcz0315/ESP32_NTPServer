#include <TinyGPSPlus.h>
#include <HardwareSerial.h>
#include <WiFi.h>
#include <esp_timer.h>

// Uncomment and monitor the system time output on the serial port
//#include <TimeLib.h>

#include "WiFiNTPServer.h"  // Introduce NTPServer library

// Set static IP information
IPAddress local_IP(*************);
IPAddress gateway(*************);
IPAddress subnet(*************);
IPAddress primaryDNS(*************);

// TinyGPSPlus instance
TinyGPSPlus gps;

// Serial connection to GPS device
HardwareSerial ss(2);

// NTPServer instance
WiFiNTPServer ntpServer("GPS", L_NTP_STRAT_PRIMARY);

// Variable to store time information
bool ppsFlag = false;
unsigned long lastPPSTime = 0;

void IRAM_ATTR ppsInterrupt() {
  // PPS signal detection, set PPS signal flag
  ppsFlag = true;
}

void setup() {
  Serial.begin(9600);
  ss.begin(9600, SERIAL_8N1, 16, 17, false);  // GPS 9600 / RX 16 / TX 17
  pinMode(13, INPUT);  // PPS13
  attachInterrupt(digitalPinToInterrupt(13), ppsInterrupt, RISING);
  lastPPSTime = (esp_timer_get_time() / 1000);  // Set lastPPSTime to the current time during initialization

  WiFi.disconnect();    // Disconnect WiFi connection
  WiFi.mode(WIFI_STA);  // Client mode
  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.setHostname("ESP_32");
  WiFi.begin("ssid", "password");

  ntpServer.begin();  // Initialize NTPServer
}

void loop() {
  // Read GPS NMEA data and pass it to TinyGPSPlus library for decoding
  while (ss.available()) {
    char c = ss.read();
    gps.encode(c);
  }

  // If a PPS signal is received and the time since the last PPS signal is less than 1500 milliseconds, update the time
  if (ppsFlag && ((esp_timer_get_time() / 1000) - lastPPSTime < 1500)) {
    // Update NTPServer reference time
    struct tm newTime;
    newTime.tm_year = gps.date.year() - 1900;  // struct tm requires year offset from 1900
    newTime.tm_mon = gps.date.month() - 1;     // struct tm requires month offset from 0
    newTime.tm_mday = gps.date.day();
    newTime.tm_hour = gps.time.hour();
    newTime.tm_min = gps.time.minute();
    newTime.tm_sec = gps.time.second();
    ntpServer.setReferenceTime(newTime, (esp_timer_get_time() / 1000));

    // Uncomment and monitor the GPS output on the serial port
    //Serial.print(c());
    
    // Uncomment and monitor the system time output on the serial port
    /*
    time_t t = mktime(&newTime);  // Convert struct tm to time_t
    setTime(t);                   // Update system time
    Serial.print("UTC date: ");
    Serial.print(year());
    Serial.print("/");
    Serial.print(month());
    Serial.print("/");
    Serial.print(day());
    Serial.print("    ");
    Serial.print("UTC time: ");
    Serial.print(hour());
    Serial.print(":");
    Serial.print(minute());
    Serial.print(":");
    Serial.println(second());
    */

    ppsFlag = false;         // Reset PPS signal flag
    lastPPSTime = millis();  // Update timestamp
  }
  ntpServer.update();  // Respond to NTP access
}
