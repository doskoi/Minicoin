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

#include <WiFiClientSecure.h>
#include <WebSocketsClient.h>

#if defined(ARDUINO_ARCH_ESP8266)
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#elif defined(ARDUINO_ARCH_ESP32)
#include <WiFi.h>
#include <WebServer.h>
#endif
#include <AutoConnect.h>

#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#define USE_SERIAL Serial

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

WebSocketsClient webSocket;

bool online;

String symbol;

float BID;
float BID_SIZE;
float ASK;
float ASK_SIZE;
float DAILY_CHANGE;
float DAILY_CHANGE_PERC;
float LAST_PRICE;
float VOLUME;
float _HIGH;
float _LOW;

void rootPage() {
  char content[] = "Hello, Trader";
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

  online = false;

  symbol = "tETHUSD";
  // Initialising the UI will init the display too.
  display.init();
  display.flipScreenVertically();
  display.setBrightness(196);

//  xTaskCreatePinnedToCore(fetchTicker, "fetchTicker", 4096, NULL, 1, NULL, ARDUINO_RUNNING_CORE);

    webSocket.beginSSL("api.bitfinex.com", 443, "/ws/2");
//    webSocket.sendTXT("{\"event\": \"subscribe\", \"channel\": \"ticker\", \"symbol\": \"tETHUSD\"}");
    webSocket.onEvent(webSocketEvent);
}

void loop()
{  
    Portal.handleClient();

    online = true;

    webSocket.loop();

    renderPrice();
}

void renderPrice() {
  // clear the display
    display.clear();
  if (LAST_PRICE == 0) {
    display.setFont(Lato_Black_16);
    display.setTextAlignment(TEXT_ALIGN_CENTER);
    display.drawString(64, 18, "Connecting...\nBitfinex");
    
    display.display();
    return;
  }
  // Title
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  display.setFont(Lato_Black_16);
  display.drawString(0, 0, "ETH/USD");
  
  // Price
  display.setTextAlignment(TEXT_ALIGN_RIGHT);
  display.drawString(128, 0, String(LAST_PRICE));
  // Chg
  display.setFont(ArialMT_Plain_10);
  display.drawString(128, 18, String( String(DAILY_CHANGE, 2) + " (" + String(DAILY_CHANGE_PERC * 100, 2) + "%)" ));
  display.setTextAlignment(TEXT_ALIGN_LEFT);
  // Vol
  display.drawString(0, 18, String("VOL\n" + String(VOLUME, 0)));
  // HIGH & LOW
  display.drawString(0, 54, String("LOW " + String(_LOW, 2) + "   HIGH " + String(_HIGH, 2)));
  // Bid & Ask
  display.drawString(0, 42, String("BID " + String(BID, 2) + "   ASK " + String(ASK, 2)));
  // Bar
  display.drawProgressBar(62, 32, 64, 8, (BID_SIZE / (ASK_SIZE + BID_SIZE) ) * 100);
  
  // write the buffer to the display
  display.display();
}

void fetchTicker(void *pvParameters) {
  while (online) {
    HTTPClient http;
    http.begin("https://api.bitfinex.com/v2/ticker/" + symbol);
    int httpCode = http.GET();

    // httpCode will be negative on error
    if(httpCode > 0) {
        // HTTP header has been send and Server response header has been handled
        Serial.printf("[HTTP] GET code: %d\n", httpCode);

        // file found at server
        if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            Serial.println(payload);
              // Allocate JsonBuffer
              // Use arduinojson.org/assistant to compute the capacity.
              const size_t capacity = JSON_ARRAY_SIZE(10) + 120;
              DynamicJsonBuffer jsonBuffer(capacity);
              // Parse JSON object
              JsonArray& root = jsonBuffer.parseArray(payload);
              if (!root.success()) {
                Serial.println(F("Parsing failed!"));
                return;
              }

              BID = root[0];
              BID_SIZE = root[1];
              ASK = root[2];
              ASK_SIZE = root[3];
              DAILY_CHANGE = root[4];
              DAILY_CHANGE_PERC = root[5];
              LAST_PRICE = root[6];
              VOLUME = root[7];
              _HIGH = root[8];
              _LOW = root[9];

              Serial.print("ETH/USD: ");
              Serial.println(String(LAST_PRICE) + "@" + String(VOLUME));
        }
    } else {
        Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
        LAST_PRICE = 0;
        VOLUME = 0;
    }

    http.end();
    delay(1000);
  }
}

void hexdump(const void *mem, uint32_t len, uint8_t cols = 16) {
  const uint8_t* src = (const uint8_t*) mem;
  USE_SERIAL.printf("\n[HEXDUMP] Address: 0x%08X len: 0x%X (%d)", (ptrdiff_t)src, len, len);
  for(uint32_t i = 0; i < len; i++) {
    if(i % cols == 0) {
      USE_SERIAL.printf("\n[0x%08X] 0x%08X: ", (ptrdiff_t)src, i);
    }
    USE_SERIAL.printf("%02X ", *src);
    src++;
  }
  USE_SERIAL.printf("\n");
}


void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {


    switch(type) {
        case WStype_DISCONNECTED:
            USE_SERIAL.printf("[WSc] Disconnected!\n");
            break;
        case WStype_CONNECTED:
            {
              USE_SERIAL.printf("[WSc] Connected to url: %s\n",  payload);

          // send message to server when Connected
              webSocket.sendTXT("{\"event\": \"subscribe\", \"channel\": \"ticker\", \"symbol\": \"tETHUSD\"}");
            }
            break;
        case WStype_TEXT:
            USE_SERIAL.printf("[WSc] get text: %s\n", payload);

      // send message to server
             webSocket.sendTXT("message");
            break;
        case WStype_BIN:
            USE_SERIAL.printf("[WSc] get binary length: %u\n", length);
            hexdump(payload, length);

            // send data to server
            // webSocket.sendBIN(payload, length);
            break;
    case WStype_ERROR:      
    case WStype_FRAGMENT_TEXT_START:
    case WStype_FRAGMENT_BIN_START:
    case WStype_FRAGMENT:
    case WStype_FRAGMENT_FIN:
      break;
    }

}
