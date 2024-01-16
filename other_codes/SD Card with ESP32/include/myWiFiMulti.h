#ifndef myWiFiMulti_H
#define myWiFiMulti_H

// Important to include all the dependencies/libraries
#include <WiFi.h>
//  Using this to connect to the strongest WiFi network
#include <WiFiMulti.h>

#define SENSE 5
#define LED 2

// Global WiFiMulti Object
extern WiFiMulti wifiMulti;

/* Global struct list for storing multiple WiFi credentials - char *SSID & char *wifiPassword */
typedef struct
{
    const char *ssid;
    const char *wifiPassword;
} WiFiCredential;
extern const WiFiCredential wifiList[];

void initWiFi();

#endif