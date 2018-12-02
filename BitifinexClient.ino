/*
 *  This sketch sends data via HTTP GET requests to data.sparkfun.com service.
 *
 *  You need to get streamId and privateKey at data.sparkfun.com and paste them
 *  below. Or just customize this script to talk to other HTTP servers.
 *
 */

#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif
#include <AutoConnect.h>

#include "font_latoblack.h"

// Include the correct display library
// For a connection via I2C using Wire include
#include <Wire.h>  // Only needed for Arduino 1.6.5 and earlier
#include "SSD1306Wire.h" // legacy include: `#include "SSD1306.h"`
// #include <SPI.h> // Only needed for Arduino 1.6.5 and earlier
// #include "SSD1306Spi.h"
// #include "SH1106SPi.h"

// Initialize the OLED display using Wire library
SSD1306Wire  display(0x3c, 22, 23);

// Initialize the OLED display using SPI
// D5 -> CLK
// D7 -> MOSI (DOUT)
// D0 -> RES
// D2 -> DC
// D8 -> CS
// SSD1306Spi        display(D0, D2, D8);

WebServer Server;
AutoConnect Portal(Server);

float btcusd;
float ethusd;

float _btcusd;
float _ethusd;

float btcvol;
float ethvol;

void rootPage() {
  char content[] = "Hello, world";
  Server.send(200, "text/plain", content);
}

void setup()
{
  Serial.begin(115200);
  delay(100);
    
  Server.on("/", rootPage);
  if (Portal.begin()) {
    Serial.println("WiFi connected: " + WiFi.localIP().toString());
  }

  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.setFont(ArialMT_Plain_10);
}

int value = 0;

void loop()
{
    Portal.handleClient();

    // clear the display
    display.clear();

    fetchBtc();
    Serial.print("BTC/USD: ");
    Serial.println(btcusd);

    display.setTextAlignment(TEXT_ALIGN_LEFT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 0, "BTC/USD");
    display.setFont(Lato_Black_16);
//    String symbol;
//    if (btcusd > _btcusd) {
//      symbol = "⬆";
//    }
//    if (btcusd < _btcusd) {
//      symbol = "⬇";
//    }
//    display.drawString(0, 20, String(String(btcusd) + symbol));
    display.drawString(0, 20, String(btcusd));
    display.setFont(ArialMT_Plain_10);
    display.drawString(0, 48, String("V: " + String(btcvol, 2)));

    fetchEth();
    Serial.print("ETH/USD: ");
    Serial.println(ethusd);
    
    display.setTextAlignment(TEXT_ALIGN_RIGHT);
    display.setFont(ArialMT_Plain_10);
    display.drawString(128, 0, "ETH/USD");
    display.setFont(Lato_Black_16);
//    if (ethusd > _ethusd) {
//      symbol = "△";
//    }
//    if (ethusd < _ethusd) {
//      symbol = "▽";
//    }
//    display.drawString(128, 20, String(String(ethusd) + symbol));
    display.drawString(128, 20, String(ethusd));
    display.setFont(ArialMT_Plain_10);
    display.drawString(128, 48, String("V: " + String(ethvol, 2)));
    
    // write the buffer to the display
    display.display();

    delay(1000);
}

void fetchBtc() {
    HTTPClient http;
    http.begin("https://api.bitfinex.com/v1/pubticker/btcusd");
    int httpCode = http.GET();

    // httpCode will be negative on error
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
//        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
//            Serial.println(payload);
              // Allocate JsonBuffer
              // Use arduinojson.org/assistant to compute the capacity.
              const size_t capacity = JSON_OBJECT_SIZE(8) + 140;
              DynamicJsonBuffer jsonBuffer(capacity);
              // Parse JSON object
              JsonObject& root = jsonBuffer.parseObject(payload);
              if (!root.success()) {
                Serial.println(F("Parsing failed!"));
                return;
              }
              _btcusd = btcusd;
              btcusd = root["last_price"].as<float>();
              btcvol = root["volume"].as<float>();
        }
    } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}

void fetchEth() {
    HTTPClient http;
    http.begin("https://api.bitfinex.com/v1/pubticker/ethusd");
    int httpCode = http.GET();

    // httpCode will be negative on error
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
//        Serial.printf("[HTTP] GET... code: %d\n", httpCode);

        // file found at server
        if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
//            Serial.println(payload);
              // Allocate JsonBuffer
              // Use arduinojson.org/assistant to compute the capacity.
              const size_t capacity = JSON_OBJECT_SIZE(8) + 140;
              DynamicJsonBuffer jsonBuffer(capacity);
              // Parse JSON object
              JsonObject& root = jsonBuffer.parseObject(payload);
              if (!root.success()) {
                Serial.println(F("Parsing failed!"));
                return;
              }
              _ethusd = ethusd;
              ethusd = root["last_price"].as<float>();
              ethvol = root["volume"].as<float>();
        }
    } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }

    http.end();
}
