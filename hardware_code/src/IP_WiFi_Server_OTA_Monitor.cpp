#include "IP_WiFi_Server_OTA_Monitor.h"

// Defining the global AsyncWebServer Object on Port 80
AsyncWebServer server(80);

// Edit WiFi Network credentials below (2.4GHz bands only)
const uint32_t connectTimeoutMs = 10000; // Connection Timeout per AP for the wifiMulti object

WiFiMulti wifiMulti;

// Add any further WiFi Credentials in this array of structures (My own structure in this corresponding header file)
const WiFiCredential wifiList[] = {
    {"Sbd-TFM", "Allthermo#2"},
    {"UB-Wifi", "Ar@#Sbd7896"},
    {"A5", "tan519690823"},
    {"POCO", "Saad123456"},
    {"BFM-Wifi", "Singer@#bfm"}};

// const char *ssid = "A5";
// const char *wifiPassword = "tan519690823";

void setIP(int espIPArray[], int gatewayArray[], int subnetArray[])
{

    // Set your Static IP address
    IPAddress local_IP(espIPArray[0], espIPArray[1], espIPArray[2], espIPArray[4]);
    // Set your Gateway IP address
    IPAddress gateway(gatewayArray[0], gatewayArray[1], gatewayArray[2], gatewayArray[3]);

    IPAddress subnet(subnetArray[0], subnetArray[1], subnetArray[2], subnetArray[3]); // - need to know subnet for the mobile hotspot too

    if (!WiFi.config(local_IP, gateway, subnet))
    {
        Serial.println("ESP32 STA Failed to configure. Re-check Gateway and Subnet of Router and assigned IP of ESP32");
    }
}

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

// void initWiFi()
// {
//     Serial.printf("Connecting to %s", ssid);
//     WiFi.begin(ssid, wifiPassword);
//     while (WiFi.status() != WL_CONNECTED)
//     {
//         delay(500);
//         Serial.print('.');
//         digitalWrite(LED, LOW);
//     }
//     Serial.println("");
//     Serial.printf("WiFi Connected to %s", WiFi.SSID());
//     digitalWrite(LED, HIGH);

//     Serial.print("ESP32 IP Address: ");
//     Serial.println(WiFi.localIP());
//     Serial.print("Gateway: ");
//     Serial.print(WiFi.gatewayIP());
//     Serial.print(" Subnet Mask: ");
//     Serial.print(WiFi.subnetMask());
//     Serial.println("");
// }

// Receives incoming messages sent from the web-based serial monitor
void recvMsg(uint8_t *data, size_t len)
{
    WebSerial.println("Received Data...");
    String d = "";
    for (int i = 0; i < len; i++)
    {
        d += char(data[i]);
    }
    // Printed on the web serial monitor using WebSerial.println(d)
    WebSerial.println(d);
}

void espOTAWebSerialEN()
{
    // Starting the ElegantOTA
    AsyncElegantOTA.begin(&server);
    Serial.println("OTA Page Created");
    // Starting the WebSerial Server
    WebSerial.begin(&server);
    // recvMsg() is registered as a callback function using the msgCallback()
    WebSerial.msgCallback(recvMsg);
    Serial.println("Web Serial Created");
    // Starting the server
    server.begin();
    Serial.println("HTTP server started");
}