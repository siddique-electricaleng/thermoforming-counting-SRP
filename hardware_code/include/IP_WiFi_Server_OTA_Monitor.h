#ifndef IP_WiFi_Server_OTA_Monitor_H
#define IP_WiFi_Server_OTA_Monitor_H

// Important to include all the dependencies/libraries
#include <Arduino.h>
#include <WiFi.h>
//  Using this to connect to the strongest WiFi network
#include <WiFiMulti.h>
// Creates the TCP connection for HTTP to sit on top of this
#include <AsyncTCP.h>
// Creates the Web Server required to display a UI
#include <ESPAsyncWebServer.h>
// Creates the functionality to upload firmware or files using the Web Server Created
#include <AsyncElegantOTA.h>
// Creates the functionality to view a serial monitor online using the Web Server
#include <WebSerial.h>
// Don't know why but there is an error occurring if I don't include the Firebase Arduino Client for ESP32 and ESP8266
// #include <Firebase_ESP_Client.h>

#define SENSE 5
#define LED 2

// Global WiFiMulti Object
extern WiFiMulti wifiMulti;

// Global struct to hold multiple WiFi credentials
// extern const char *ssid;
// extern const char *wifiPassword;

struct WiFiCredential
{
    const char *ssid;
    const char *wifiPassword;
};
extern const WiFiCredential wifiList[];

// Connection Timeout for each Access Point
extern const uint32_t connectTimeoutMs;

// Declaring/Creating the AsyncWebServer Object as a Global Object - Defined in the IP_WiFi_Server_OTA_Monitor.cpp file
extern AsyncWebServer server;

void setIP(int espIPArray[], int gatewayArray[], int subnetArray[]);
void initWiFi();
void espOTAWebSerialEN();

#endif