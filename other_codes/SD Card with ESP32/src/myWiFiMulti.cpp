#include "myWiFiMulti.h"

// Edit WiFi Network credentials below (2.4GHz bands only)
WiFiMulti wifiMulti;

// Add any further WiFi Credentials in this array of structures (My own structure in this corresponding header file)
const WiFiCredential wifiList[] = {
    {"Sbd-TFM", "Allthermo#2"},
    {"UB-Wifi", "Ar@#Sbd7896"},
    {"A5", "tan519690823"},
    {"POCO C55", "Saad123456"},
    {"BFM-Wifi", "Singer@#bfm"}};

/* Setup Function */
// Using WiFiMulti's wifiMulti object to connect to the strongest WiFi network from a list of networks
void initWiFi()
{
    WiFi.mode(WIFI_STA);
    int savedNet = (sizeof(wifiList) / sizeof(wifiList[0]));
    Serial.printf("The number of saved WiFi networks is %d\n", savedNet);
    // Loop through the wifiList array of structures and add them to the wifiMulti object
    for (int i = 0; i < savedNet; ++i)
    {
        wifiMulti.addAP(wifiList[i].ssid, wifiList[i].wifiPassword);
    }

    // Scanning through all the available WiFi networks from the wifiMulti object
    int n = WiFi.scanNetworks();
    Serial.printf((n == 0) ? "No networks found" : "%d Networks found\n", n);

    Serial.println("Connecting WiFi");

    while (wifiMulti.run() != WL_CONNECTED)
    {
        digitalWrite(LED, LOW);
        Serial.print(".");
        delay(500);
    }

    Serial.printf("\nConnected to %s\n", WiFi.SSID());
    digitalWrite(LED, HIGH);
    Serial.print("ESP32 IP Address: ");
    Serial.println(WiFi.localIP());

    // Get to know some information of the Gateway and the subnetMask of the WiFi

    Serial.printf("%s ", WiFi.SSID());
    Serial.print("Gateway: ");
    Serial.print(WiFi.gatewayIP());
    Serial.print(" Subnet Mask: ");
    Serial.print(WiFi.subnetMask());
    Serial.println("");
}