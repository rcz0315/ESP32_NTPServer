#include <WiFi.h>
#include <HardwareSerial.h>
#include <esp_timer.h>
#include <TinyGPSPlus.h>

#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <AsyncTCP.h>
#include <ArduinoOTA.h>

// Uncomment and monitor the system time output on the serial port
// #include <TimeLib.h>

// Current version
String versionString = "********";

#include "WiFiNTPServer.h"  // Introduce NTPServer library
#include "html.h"           // Include the HTML header

// Set Wi-Fi information
const char* ssid = "********";
const char* password = "********";
IPAddress local_IP(********);
IPAddress gateway(********);
IPAddress subnet(********);
IPAddress primaryDNS(********);

// TinyGPSPlus instance
TinyGPSPlus gps;

// Serial connection to GPS device
HardwareSerial ss(2);

struct tm newTime;
struct timeval tv;

// NTPServer instance
WiFiNTPServer ntpServer("GPS", L_NTP_STRAT_PRIMARY);

// Web server instance
AsyncWebServer server(80);

// Variable to store time information
bool ppsFlag = false;
int64_t lastPPSTime = 0;

// Buffer to store GPS NMEA sentences
String nmeaSentences[25];
int nmeaIndex = 0;

// Variables to store status information
String deviceStatus = "Initializing";

void IRAM_ATTR ppsInterrupt() {
  // PPS signal detection, set PPS signal flag
  lastPPSTime = esp_timer_get_time();  // Set lastPPSTime to the current time during initialization
  ppsFlag = true;
}

void gpsTask(void *parameter) {
  for (;;) {
    // Read GPS NMEA data and pass it to TinyGPSPlus library for decoding
    while (ss.available()) {
      char c = ss.read();
      gps.encode(c);
      if (c == '\n') {
        nmeaSentences[nmeaIndex] = ss.readStringUntil('\n');
        nmeaIndex = (nmeaIndex + 1) % 25;
      }
    }

    // If a PPS signal is received and the time since the last PPS signal is less than 1500 milliseconds, update the time
    if (ppsFlag && (esp_timer_get_time() - lastPPSTime < 1500)) {
      // Update NTPServer reference time
      newTime.tm_year = gps.date.year() - 1900;
      newTime.tm_mon = gps.date.month() - 1;
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
      
      // Uncomment and monitor the GPS output on the serial port
      //Serial.print(c);

      // Uncomment and monitor the system time output on the serial port
      /*
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

      ppsFlag = false;                     // Reset PPS signal flag
      lastPPSTime = esp_timer_get_time();  // Update timestamp
    }

    // Delay to prevent tasks from taking up too much CPU time. Only gpsTask, this item is meaningless
    // vTaskDelay(10 / portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(9600);
  ss.begin(9600, SERIAL_8N1, 16, 17, false);  // GPS 9600 / RX 16 / TX 17
  pinMode(13, INPUT);                         // PPS13
  attachInterrupt(digitalPinToInterrupt(13), ppsInterrupt, RISING);

  WiFi.disconnect();    // Disconnect WiFi connection
  WiFi.mode(WIFI_STA);  // Client mode
  WiFi.config(local_IP, gateway, subnet, primaryDNS);
  WiFi.setHostname("ESP_32");
  WiFi.begin(ssid, password);
  
  xTaskCreatePinnedToCore(
    gpsTask,    // Task function
    "gpsTask",  // Task name
    4096,       // Task stack size
    NULL,       // Task parameters
    1,          // Task priority
    NULL,       // Task handle
    1           // Specify running on Core 1
  );

  ntpServer.begin();  // Initialize NTPServer

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // OTA Setup
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else {  // U_SPIFFS
      type = "filesystem";
    }
    deviceStatus = "Start updating " + type;
  });
  ArduinoOTA.onEnd([]() {
    deviceStatus = "Update finished";
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    deviceStatus = "Progress: " + String((progress / (total / 100))) + "%";
  });
  ArduinoOTA.onError([](ota_error_t error) {
    deviceStatus = "Error[" + String(error) + "]: ";
    if (error == OTA_AUTH_ERROR) {
      deviceStatus += "Auth Failed";
    } else if (error == OTA_BEGIN_ERROR) {
      deviceStatus += "Begin Failed";
    } else if (error == OTA_CONNECT_ERROR) {
      deviceStatus += "Connect Failed";
    } else if (error == OTA_RECEIVE_ERROR) {
      deviceStatus += "Receive Failed";
    } else if (error == OTA_END_ERROR) {
      deviceStatus += "End Failed";
    }
  });
  ArduinoOTA.begin();

  // Serve the index.html
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", index_html);
  });

  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "image/x-icon", "");
  });

  // Handle /data endpoint
  server.on("/data", HTTP_GET, [](AsyncWebServerRequest *request) {
    String timeString = String(gps.date.year()) + "/" + String(gps.date.month()) + "/" + String(gps.date.day()) + " " + String(gps.time.hour()) + ":" + String(gps.time.minute()) + ":" + String(gps.time.second());

    String nmeaData = "";
    for (int i = 0; i < 25; i++) {
      nmeaData += nmeaSentences[i] + "\n";
    }

    // Create a JSON object
    DynamicJsonDocument jsonDoc(512);
    jsonDoc["versionString"] = versionString;
    jsonDoc["timeString"] = timeString;
    jsonDoc["nmeaData"] = nmeaData;

    // A JSON response occurs
    String jsonResponse;
    serializeJson(jsonDoc, jsonResponse);
    request->send(200, "application/json", jsonResponse);
  });

  // Handle /restart endpoint
  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Restarting...");
    delay(500);  // Wait for the response to be sent
    ESP.restart();
  });

  // OTA Endpoint
  server.on(
    "/update", HTTP_POST, [](AsyncWebServerRequest *request) {
      // Do not send a response here
    },
    [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t length, bool final) {
      if (!index) {
        Serial.printf("Update Start: %s\n", filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
          Update.printError(Serial);
        }
      }
      if (length) {
        if (Update.write(data, length) != length) {
          Update.printError(Serial);
        }
      }
      if (final) {
        if (Update.end(true)) {
          Serial.printf("Update Success: %uB\n", index + length);
          request->send(200, "text/plain", "Update successful. Restarting...");
          Serial.flush();
          delay(1000);
          ESP.restart();
        } else {
          Update.printError(Serial);
          request->send(500, "text/plain", "Update failed!");
        }
      }
    });
  server.begin();
}

void loop() {
  ntpServer.update();  // Respond to NTP access
  ArduinoOTA.handle();
}
