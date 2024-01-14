#include "IP_WiFi_Server_OTA_Monitor.h"
#include <stdio.h>
#include "time.h"

#include <ESP_Mail_Client.h>

// #include <ESP_Google_Sheet_Client.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

// For OTA type in a browser: <IP Address of ESP32>/update e.g. 192.168.1.114/update

// To access the Web Serial using the web server, type in the browser : <IP Address>/webserial

// // Get the digital logic of the AC input to solenoid valve in Thermoforming (Converted using the circuit)
int digVal = 0;

const char *ntpServer = "pool.ntp.org";
const long gmtOffset_sec = 6 * 3600;
const int daylightOffset_sec = 0;
struct tm timeinfo;

// --------------------------------------------- Body Counting Variables are below ------------------------------------------------
// Date Variables
int todayDateCheck = -1;
int nowYear;
int nowMonth;
int nowDay;
String dateString;

// Time Variables
int lastHrVar = -1;
int nowHr;
int nowMin;
int nowSec;
String todayTimeStart;
String bodyRecTimeStart;
String bodyRecTimeStop;
unsigned long continuousTime = 0; // to store the Detection Time continuously

// const int contTimeInterval = 150; // 0.5s for the continuousTime var. to cross this
const int interval = 20; // 0.02 seconds

// ------------------------------------------------ Production Related Variables Below-------------------------------------------------
// Body Count variables
int hourlyBodyCtr, totalBodyCtr = 0, prevTotalBodyCt = 0;
// Debugging Purposes
int tempHourlyBodyCtr = 0;
// Counts how many times the solenoid has been activated
int activationCount = 0; // The solenoid is activated 2x for 1 liner. So this variable should have a maximum value of 2.
unsigned long bodyCountStart;
bool bodyCountFlag;
// ------------------------------------------------ Production Related Variables Above-------------------------------------------------

// ---------------------------------------- Variables to Use IFTTT to write in Google Sheet --------------------------------------------
// unique IFTTT URL resource obtaied from the Documentation of WebHooks in IFTTT
const char *countsURLResource = "/trigger/thermoDataStore/with/key/o6lD7agRVP_KC_fKE2G6gE9qAjubXj6rGaU-WhxAtCR";
const char *dateTimeMsgURLResource = "/trigger/thermoData_lastDateAndHr/with/key/o6lD7agRVP_KC_fKE2G6gE9qAjubXj6rGaU-WhxAtCR";
// Maker Webhooks IFTTT
const char *serverIFTTT = "maker.ifttt.com";
// For Sending Hourly-Count | Total-Count | Previous-Hour-Total-Counts
void makeIFTTTRequest(const char *appletURL, int val1, int val2, int val3);
// For Sending LastUploadedDate and LastUploadedHr
void makeMsgIFTTTRequest(const char *appletURL, String val1, String val2, String val3);
// ------------------------------------------------ Google Sheet variables ------------------------------------------

String COUNTS_SCRIPT_ID = "AKfycbwWipi9yFI1WcMXF2jfqMNzFirN-udAwd_Thz1DjoCHMcv8fcZLCnHSasytgp9I8jq6Zw";
String DATE_TIME_MSG_SCRIPT_ID = "AKfycbw8GuHFfn3fTqxvbgVsUBqZ6xrbZ4VlrX9A96iaSElBh4Hcfn8KbbMe9ugLJ3RbqfNqow";
String read_spreadsheet(String scriptID);
String storedCountsMsg, storedDateTime;

// ------------------------------------------------ Email Notification Codes Below ----------------------------------
/** The smtp host name e.g. smtp.gmail.com for GMail or smtp.office365.com for Outlook or smtp.mail.yahoo.com */
#define SMTP_HOST "smtp.gmail.com"
#define SMTP_PORT 465

/* The sign in credentials */
#define AUTHOR_EMAIL "thermoformingreporter@gmail.com"
#define AUTHOR_PASSWORD "dqzj ckuz vqnu vtyx"

/* Declare the global used SMTPSession object for SMTP transport */
SMTPSession smtp;

/* Declare the global Session_Config for user defined session credentials */
Session_Config config;

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status);

int thermoMachineNo;

/* Callback function to send the email */
void sendMsg(int machineNo, String dateVal, String timeStart, String timeStop, int units, int totalUnits, int pastTotalUnits);
size_t msgNo;

/* Mailing List*/
const String RECIPIENT_EMAILS[] = {"AbuBakr.Siddique@singerbd.com"};
const String RECIPIENT_NAMES[] = {"Abu Bakr Siddique"};

// Initial default HTML Message
String htmlMsg =
    "<div>"
    "<style>"
    "table {"
    "font-family: arial, sans-serif;"
    "border-collapse: collapse;"
    "width: 100%;"
    "}"

    "td, th {"
    "border: 1px solid #dddddd;"
    "text-align: left;"
    "padding: 8px;"
    "}"

    "tr:nth-child(even) {"
    "background-color: #dddddd;"
    "}"
    "</style>"
    "<p> [TODAY DATE]: Total Liners Completed upto now = 0 units</p>"
    "<table style=\"width:100%\">"
    "<tr>"
    "<th>Date</th>"
    "<th>Start Time</th>"
    "<th>End Time</th>"
    "<th>Production (units)</th>"
    "<th>Target</th>"
    "<th>Delta</th>"
    "</tr>"
    "</table>"
    "</div>";
// ------------------------------------------------ Email Notification Codes Above ----------------------------------

// ------------------------------------------------- Function prototypes below ---------------------------------------
/* Callback function to get the date in the correct format */
String dateFormat(int yearVal, int monthVal, int dayVal);
/* Callback function to get the time in the correct format */
String timeFormat(int hrVal, int minVal, int secVal);
/* Callback function to continously detect the bodies which have been produced*/
void bodyCountFunc();
// -------------------------------------------------- Function prototypes above --------------------------------------

void setup()
{
  pinMode(LED, OUTPUT);
  pinMode(SENSE, INPUT);
  bodyCountFlag = 1;
  /* Selecting the Machine No based on the connection to the ESP32 pin*/
  switch (SENSE)
  {
  case 5:
    thermoMachineNo = 3; // cornermost Body Liner Machine (furthest from entrance)
    break;
  case 18:
    thermoMachineNo = 2;
    break;
  case 19:
    thermoMachineNo = 4; // cornermost Body Liner Machine (furthest from entrance)
    break;
  case 25:
    thermoMachineNo = 1; // Door Liner
    break;
  }
  // Setting Serial Monitor to monitor output
  Serial.begin(115200);
  // Setting custom/Static IP Address
  // setIP(setEspIP, gatewayValRouter, subnetValRouter);
  // --------------------------------------------- WiFi Connection Request ✓ ------------------------------------------------------
  // Connect to WiFi
  initWiFi();
  // --------------------------------------------- WiFi Connection Request ✓ ------------------------------------------------------
  // -------------------------------------------- Turning on OTA and WebSerial ----------------------------------------------------
  // Setup the home page of the web server
  // Handles client requests, and shows this in the root URL, which is basically the IP address
  // Turns on the Web Server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", "Hi! I am ESP32."); });
  Serial.println("Home page created");

  // // Setting up the web serial and OTA functionalities
  espOTAWebSerialEN();
  // // System is Active Msg
  WebSerial.println("System is Active");
  // -------------------------------------------- The OTA and WebSerial are online ✓ ----------------------------------------------

  // ----------------------------------------------------- Time Variables ✓ -------------------------------------------------------
  // Init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time. Trying to obtain it ");
    delay(500);
    while (!getLocalTime(&timeinfo))
    {
      delay(500);
      Serial.print(".");
    }
    return;
  }
  // ---------------------------------------------- Storing the time and date variables ------------------------------------------
  nowYear = timeinfo.tm_year + 1900; // Year (Gives since 1900 so we need to add 1900 to that value to get current year)
  nowMonth = timeinfo.tm_mon + 1;    // Month
  nowDay = timeinfo.tm_mday;         // Day
  todayDateCheck = nowDay;           // Storing the Day to later confirm if we are within the same or different day

  nowHr = timeinfo.tm_hour; // Hour
  nowMin = timeinfo.tm_min; // Min
  nowSec = timeinfo.tm_sec; // Second
  lastHrVar = nowHr;        // Storing the hour to confirm if we are within the same or different day
  // lastHrVar = nowMin;
  todayTimeStart = timeFormat(nowHr, nowMin, nowSec);
  bodyRecTimeStop = todayTimeStart; // Stores the time when the counting for that particular hour begins (e.g. between 3-4pm, this stores the 3pm)
  dateString = dateFormat(nowYear, nowMonth, nowDay);
  Serial.printf("Date:%s Time: %s\n", dateString, todayTimeStart);
  // ----------------------------------------------------- Time Variables Above --------------------------------------------------

  /* ------------------------------------------------------ Email Setup Below ----------------------------------------------------*/
  /*  Set the network reconnection option */
  MailClient.networkReconnect(true);

  /** Enable the debug via Serial port
   * 0 for no debugging
   * 1 for basic level debugging
   *
   * Debug port can be changed via ESP_MAIL_DEFAULT_DEBUG_PORT in ESP_Mail_FS.h
   */
  smtp.debug(1);

  /* Set the callback function to get the sending results */
  smtp.callback(smtpCallback);

  /* Set the session config */
  config.server.host_name = SMTP_HOST;
  config.server.port = SMTP_PORT;
  config.login.email = AUTHOR_EMAIL;
  config.login.password = AUTHOR_PASSWORD;
  config.login.user_domain = "";

  // config.time.ntp_server = F("pool.ntp.org,time.nist.gov");
  // config.time.gmt_offset = 6;
  // config.time.day_light_offset = 0;
  /* ------------------------------------------------------ Email Setup Above ----------------------------------------------------*/

  /* ------------------------------------------------- Reading Functionality Below ---------------------------------------------- */
  // Note: Please don't read in loops it causes the ESP32 to run out of RAM
  storedCountsMsg = read_spreadsheet(COUNTS_SCRIPT_ID);
  // Serial.printf("The Counts reading from Google Sheets is %s\n", storedCountsMsg);
  Serial.print("The JSON response for stored Counts Message is: ");
  Serial.println(storedCountsMsg);

  int getFirstSpaceCounts, getSecondSpaceCounts;
  String secondSpacePos;
  int getLastHrCount, getLastTotalCount, getLastPrevTotalCount;

  getFirstSpaceCounts = storedCountsMsg.indexOf(' ');
  if (getFirstSpaceCounts != -1)
  {
    getLastHrCount = storedCountsMsg.substring(0, getFirstSpaceCounts).toInt();
    secondSpacePos = storedCountsMsg.substring(getFirstSpaceCounts + 1);
  }

  getSecondSpaceCounts = secondSpacePos.indexOf(' ');
  if (getSecondSpaceCounts != -1)
  {
    getLastTotalCount = secondSpacePos.substring(0, getSecondSpaceCounts).toInt();
    getLastPrevTotalCount = secondSpacePos.substring(getSecondSpaceCounts + 1).toInt();
  }
  Serial.printf("lastHrCount: %d lastTotalCount: %d prevTotalCount: %d\n", getLastHrCount, getLastTotalCount, getLastPrevTotalCount);
  delay(1000); // Very important to have this delay otherwise, we are sending the requests from ESP32 to read 2 different sheets too fast
  storedDateTime = String(read_spreadsheet(DATE_TIME_MSG_SCRIPT_ID));
  Serial.printf("The JSON stored previous Date and Time is %s\n", storedDateTime);
  int spacePosGetDateTime = storedDateTime.indexOf(' ');
  String getLastDate;
  int getLastHr;

  if (spacePosGetDateTime != -1)
  {
    getLastDate = storedDateTime.substring(0, spacePosGetDateTime);
    getLastHr = storedDateTime.substring(spacePosGetDateTime + 1).toInt();
  }
  Serial.printf("The getDate is %s and the getHr is %d\n", getLastDate, getLastHr);

  if (getLastDate == dateString)
  {
    if (getLastHr != nowHr)
    {
      // send email code
      sendMsg(thermoMachineNo, getLastDate, timeFormat(getLastHr, 0, 0), timeFormat((getLastHr + 1), 0, 0), getLastHrCount, getLastTotalCount, getLastPrevTotalCount);
      prevTotalBodyCt = getLastTotalCount;
      makeMsgIFTTTRequest(dateTimeMsgURLResource, getLastDate, String(nowHr), "");
    }
    else if (getLastHr == nowHr)
    {
      if ((getLastHrCount > 0) || (getLastTotalCount > 0) || (getLastPrevTotalCount > 0))
      {
        hourlyBodyCtr = getLastHrCount;
        totalBodyCtr = getLastTotalCount;
        prevTotalBodyCt = getLastPrevTotalCount;
      }
    }
  }

  /* ------------------------------------------------- Reading Functionality Above ---------------------------------------------- */
}

void loop()
{
  if (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(LED, LOW);
    while (WiFi.status() != WL_CONNECTED)
    {
      initWiFi();
    }
  }
  if (!getLocalTime(&timeinfo))
  {
    // Serial.println("Failed to obtain time. Please resolve it. Suggestion: Check the internet availability");
    WebSerial.println("Failed to obtain time. Please resolve it. Suggestion: Check the internet availability");
    return;
  }
  else
  {
    bodyRecTimeStart = bodyRecTimeStop;
    ; // NULL Statement
      //--------------------------------------------- Body detection code/function ----------------------------------------
    // Serial.printf("The body count function has been reinitiated after %d ms\n", (millis() - bodyCountStart));
    if (bodyCountFlag == 1)
    {
      bodyCountFunc(); // allowed to count the body
    }
    if (millis() - bodyCountStart >= 15000)
    {
      bodyCountFlag = 1;
    }
    // Increments hourlyBodyCtr & totalBodyCtr
    // Reset the activation count if it is more than 0, for 1 minute

    // -------------------------------------------- Some Timer Variables ------------------------------------------------
    // Get Current Time
    nowHr = timeinfo.tm_hour;
    nowMin = timeinfo.tm_min;
    nowSec = timeinfo.tm_sec;

    // updating Current Date if we are recording longer than a day
    // Capture Current Date
    nowYear = timeinfo.tm_year + 1900;
    nowMonth = timeinfo.tm_mon + 1;
    nowDay = timeinfo.tm_mday;
    if (todayDateCheck != nowDay)
    {
      // Refresh the Date
      dateString = dateFormat(nowYear, nowMonth, nowDay);
      // Refresh the total Body Counter
      totalBodyCtr = 0;
      prevTotalBodyCt = 0;
      tempHourlyBodyCtr = 0;
      makeIFTTTRequest(countsURLResource, hourlyBodyCtr, totalBodyCtr, prevTotalBodyCt);
      makeMsgIFTTTRequest(dateTimeMsgURLResource, dateString, String(nowHr), "");
      // Refresh Message Number
      msgNo = 0;
      // Refresh the HTML message list
      htmlMsg =
          "<div>"
          "<style>"
          "table {"
          "font-family: arial, sans-serif;"
          "border-collapse: collapse;"
          "width: 100%;"
          "}"

          "td, th {"
          "border: 1px solid #dddddd;"
          "text-align: left;"
          "padding: 8px;"
          "}"

          "tr:nth-child(even) {"
          "background-color: #dddddd;"
          "}"
          "</style>"
          "<table style=\"width:100%\">"
          "<tr>"
          "<th>Date</th>"
          "<th>Start from</th>"
          "<th>to</th>"
          "<th>units</th>"
          "</tr>"
          "</table>"
          "</div>";
    }
    /* Serial Print only if a body is detected */
    if (tempHourlyBodyCtr != hourlyBodyCtr)
    {
      Serial.printf("\nHourly Body Count outside email: %d\n", hourlyBodyCtr);
      WebSerial.printf("\nHourly Body Count outside email: %d\n", hourlyBodyCtr);
      Serial.printf("Total Body Count outside email: %d\n", totalBodyCtr);
      WebSerial.printf("Total Body Count outside email: %d\n", totalBodyCtr);
    }
    // -------------------------------------------- Sending Email (every hr OR 4.30 pm) ---------------------------------

    if ((lastHrVar != nowHr) || ((nowHr == 16) && (nowMin == 30) && (nowSec == 0)))
    {
      Serial.printf("------------------------------ SENDING EMAIL NOW -----------------------------\n");
      WebSerial.printf("------------------------------ SENDING EMAIL NOW -----------------------------\n");
      lastHrVar = nowHr;
      // lastHrVar = nowMin;
      bodyRecTimeStop = timeFormat(nowHr, nowMin, nowSec);
      // ------------------------ Invoking Email Reporting Function -------
      sendMsg(thermoMachineNo, dateString, bodyRecTimeStart, bodyRecTimeStop, hourlyBodyCtr, totalBodyCtr, prevTotalBodyCt);
      // ------------------------ End of Email Reporting Function ---------
      prevTotalBodyCt = totalBodyCtr;
      makeMsgIFTTTRequest(dateTimeMsgURLResource, dateString, String(nowHr), ""); // having an error here

      Serial.println("\nAn hour has passed");
      WebSerial.println("\nAn hour has passed");
      // Printing time, body counts/hr and total body counts
      Serial.printf("Time: %02d:%02d:%02d\n", nowHr, nowMin, nowSec);
      Serial.printf("Hourly Body Count is %d\n", hourlyBodyCtr);
      Serial.printf("Total.... Body Count is %d\n", totalBodyCtr);
      // reset the hourlyBodyCtr variable to 0
      hourlyBodyCtr = 0;
      makeIFTTTRequest(countsURLResource, hourlyBodyCtr, totalBodyCtr, prevTotalBodyCt);
    }
    tempHourlyBodyCtr = hourlyBodyCtr;
  }
}

// ---------------------------------------------------- Functions (Don't touch once finalized) ----------------------------------
void bodyCountFunc()
{
  if (digitalRead(SENSE) == HIGH)
  {
    bodyCountStart = millis();
    // Serial.println("The valve is now active");
    WebSerial.println("The valve is now active");
    unsigned long prevMillis = millis();
    while (digitalRead(SENSE) == HIGH)
    {
      // This is some stuff just to make it look like full
    }
    unsigned long currentMillis = millis();
    // if ((currentMillis - prevMillis) != 0)
    // {
    //   Serial.printf("Difference between Millis is %d\n", (currentMillis - prevMillis));
    //   WebSerial.printf("Difference between Millis is %d\n", (currentMillis - prevMillis));
    // }
    continuousTime += (currentMillis - prevMillis);
    if (continuousTime != 0)
    {
      Serial.printf("Hr:Min:Sec ---> %d:%d:%d\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
      Serial.printf("\nContinuous Time from Detection is %d ms\n", continuousTime);
      WebSerial.printf("\nContinuous Time from Detection is %d ms\n", continuousTime);
    }
    if ((currentMillis - prevMillis >= interval))
    {
      if ((digitalRead(SENSE) == LOW))
      {
        WebSerial.println("Body was detected by normal time difference");
        continuousTime = 0; // reset the continuousTime interval
        ++hourlyBodyCtr;
        ++totalBodyCtr;
        // Serial.printf("Hourly Bodies: %d\nTotal Bodies: %d\n", hourlyBodyCtr, totalBodyCtr);
        WebSerial.printf("Hourly Bodies: %d\nTotal Bodies: %d\n", hourlyBodyCtr, totalBodyCtr);
        // --------------------------------- Sending the total and hourly Body Count in Google Sheets ------------------
        makeIFTTTRequest(countsURLResource, hourlyBodyCtr, totalBodyCtr, prevTotalBodyCt);
        bodyCountFlag = 0;
      }
    }
    else if ((continuousTime >= interval))
    {
      if ((digitalRead(SENSE) == LOW))
      {
        // Serial.println("Body was detected by continuous Time");
        WebSerial.println("Body was detected by continuous Time");
        continuousTime = 0; // reset the continuousTime interval
        ++hourlyBodyCtr;
        ++totalBodyCtr;
        // Serial.printf("Hourly Bodies: %d\nTotal Bodies: %d\n", hourlyBodyCtr, totalBodyCtr);
        WebSerial.printf("Hourly Bodies: %d\nTotal Bodies: %d\n", hourlyBodyCtr, totalBodyCtr);
        // --------------------------------- Sending the total and hourly Body Count in Google Sheets ------------------
        makeIFTTTRequest(countsURLResource, hourlyBodyCtr, totalBodyCtr, prevTotalBodyCt);
        bodyCountFlag = 0;
      }
    }
  }
}

void makeIFTTTRequest(const char *appletURL, int val1, int val2, int val3)
{
  // Serial.print("Connecting to ");
  WebSerial.print("Connecting to ");
  // Serial.println(serverIFTTT);
  WebSerial.println(serverIFTTT);

  WiFiClient client;
  int retries = 5;
  // Tries connecting to the MakerIFTTT.com server with 5 retries
  while (!!!client.connect(serverIFTTT, 80) && (retries-- > 0))
  {
    Serial.print(".");
  }
  // This is what it'll print if client connection fails ie the whole thing fails to connect to IFTTT
  Serial.println();
  if (!!!client.connected())
  {
    Serial.println("Failed to connect to IFTTT...");
    WebSerial.println("Failed to connect to IFTTT...");
  }

  Serial.print("Request resource: ");
  Serial.println(appletURL); // This contains the URL for sending the data/values
  int value1 = val1;
  int value2 = val2;
  int value3 = val3;

  // The JSON object that we are going to send
  String jsonObject;
  if ((value2 == 0) && (value3 == 0))
  {
    jsonObject = String("{\"value1\":\"") + val1 + "\",\"value2\":\"" + "" + "\",\"value3\":\"" + "" + "\"}";
  }
  else if ((value2 != 0) && (value3 == 0))
  {
    jsonObject = String("{\"value1\":\"") + val1 + "\",\"value2\":\"" + val2 + "\",\"value3\":\"" + "" + "\"}";
  }
  else
  {
    jsonObject = String("{\"value1\":\"") + val1 + "\",\"value2\":\"" + val2 + "\",\"value3\":\"" + val3 + "\"}";
  }

  client.println(String("POST ") + appletURL + " HTTP/1.1");
  client.println(String("Host: ") + serverIFTTT);
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);

  // All of the above sends the follwing thing which completes the HTTP request with the JSON payload:

  // POST /trigger/thermoDataStore/with/key/o6lD7agRVP_KC_fKE2G6gE9qAjubXj6rGaU-WhxAtCR HTTP/1.1
  // Host: maker.ifttt.com
  // Connection: close
  // Content-Type: application/json
  // Content-Length: 75

  // {
  //   "value1": "value1",
  //   "value2": "value2",
  //   "value3": "value3"
  // }
  // ------------------------ The above is the JSON stuff that is sent ---------------------------
  int timeout = 5 * 10; // 5 seconds
  while (!!!client.available() && (timeout-- > 0))
  {
    delay(100);
  }
  if (!!!client.available())
  {
    Serial.println("No response...");
  }
  while (client.available())
  {
    Serial.write(client.read());
  }

  // Serial.println("\nclosing connection");
  WebSerial.println("\nclosing connection");
  client.stop();
}

void makeMsgIFTTTRequest(const char *appletURL, String val1, String val2, String val3)
{
  // Serial.print("Connecting to ");
  WebSerial.print("Connecting to ");
  // Serial.println(serverIFTTT);
  WebSerial.println(serverIFTTT);

  WiFiClient client;
  int retries = 5;
  // Tries connecting to the MakerIFTTT.com server with 5 retries
  while (!!!client.connect(serverIFTTT, 80) && (retries-- > 0))
  {
    Serial.print(".");
  }
  // This is what it'll print if client connection fails ie the whole thing fails to connect to IFTTT
  Serial.println();
  if (!!!client.connected())
  {
    Serial.println("Failed to connect to IFTTT...");
    WebSerial.println("Failed to connect to IFTTT...");
  }

  Serial.print("Request res0ource: ");
  Serial.println(appletURL); // This contains the URL for sending the data/values
  String value1 = val1;
  String value2 = val2;
  String value3 = val3;

  // The JSON object that we are going to send
  String jsonObject;
  jsonObject = String("{\"value1\":\"") + val1 + "\",\"value2\":\"" + val2 + "\",\"value3\":\"" + val3 + "\"}";

  client.println(String("POST ") + appletURL + " HTTP/1.1");
  client.println(String("Host: ") + serverIFTTT);
  client.println("Connection: close\r\nContent-Type: application/json");
  client.print("Content-Length: ");
  client.println(jsonObject.length());
  client.println();
  client.println(jsonObject);

  // All of the above sends the follwing thing which completes the HTTP request with the JSON payload:

  // POST /trigger/thermoDataStore/with/key/o6lD7agRVP_KC_fKE2G6gE9qAjubXj6rGaU-WhxAtCR HTTP/1.1
  // Host: maker.ifttt.com
  // Connection: close
  // Content-Type: application/json
  // Content-Length: 75

  // {
  //   "value1": "value1",
  //   "value2": "value2",
  //   "value3": "value3"
  // }
  // ------------------------ The above is the JSON stuff that is sent ---------------------------
  int timeout = 5 * 10; // 5 seconds
  while (!!!client.available() && (timeout-- > 0))
  {
    delay(100);
  }
  if (!!!client.available())
  {
    Serial.println("No response...");
  }
  while (client.available())
  {
    Serial.write(client.read());
  }

  // Serial.println("\nclosing connection");
  WebSerial.println("\nclosing connection");
  client.stop();
}

// Reads the spreadsheet data - I'm trying to customize this reading for everything
String read_spreadsheet(String scriptID)
{
  HTTPClient http;
  String url = "https://script.google.com/macros/s/" + scriptID + "/exec?read";
  WebSerial.println("Making a read request to Google Sheets");
  http.begin(url.c_str());
  http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
  int httpCode = http.GET();
  String returnVal;
  if (httpCode > 0)
  {
    returnVal = http.getString();
  }
  else
  {
    WebSerial.println("Error on HTTP request");
  }
  http.end();
  return returnVal;
}

// Sends email notifictions
void sendMsg(int machineNo, String dateVal, String timeStart, String timeStop, int units, int totalUnits, int pastTotalUnits)
{
  /* Declare the message class */
  SMTP_Message message;
  char senderName[30];
  char emailSubj[70];
  // Storing the Target and the Delta integers for calculation later in the bodyCountFunc()
  int targetLiner = 0, deltaLiner = 0;
  if (machineNo != 2)
  {
    sprintf(senderName, "Body Liner Counter #%d", machineNo);
    sprintf(emailSubj, "Body Liners Produced in Thermoforming Machine %d", machineNo);
    targetLiner = 22;
    deltaLiner = (units - targetLiner);
  }
  else
  {
    sprintf(senderName, "Door Liner Counter #%d", machineNo);
    sprintf(emailSubj, "Door Liners Produced in Thermoforming Machine %d", machineNo);
    targetLiner = 53;
    deltaLiner = (units - targetLiner);
  }

  /* Set the message headers */
  message.sender.name = senderName;
  message.sender.email = AUTHOR_EMAIL;
  message.subject = emailSubj;
  message.addRecipient(RECIPIENT_NAMES[0], RECIPIENT_EMAILS[0]);

  // ------------------------------------- Modified Code -------------------------------------
  /*Modify HTML message*/
  htmlMsg.replace("</table>", String(""));
  htmlMsg.replace("</div>", String(""));
  htmlMsg +=
      "<tr>"
      "<td>[TABLE DATE]</td>"
      "<td>[START TIME]</td>"
      "<td>[END TIME]</td>"
      "<td>[PRODUCED UNITS]</td>"
      "<td>[TARGET UNITS]</td>"
      "<td>[DELTA UNITS]</td>"
      "</tr>"
      "</table>"
      "</div>";
  // ------------------------------------- Modified Code -------------------------------------
  // ------------------------------------- Added Code -------------------------------------
  // Replacing the contents inside the [] with actual values
  // For the first message above the table
  htmlMsg.replace("[TODAY DATE]", dateVal);
  htmlMsg.replace((String(pastTotalUnits) + " units"), (String(totalUnits) + " units"));

  htmlMsg.replace("[TABLE DATE]", dateVal);
  htmlMsg.replace("[START TIME]", timeStart);
  htmlMsg.replace("[END TIME]", timeStop);
  htmlMsg.replace("[PRODUCED UNITS]", String(units));
  htmlMsg.replace("[TARGET UNITS]", String(targetLiner));
  htmlMsg.replace("[DELTA UNITS]", String(deltaLiner));

  // ------------------------------------- Added Code -------------------------------------
  message.html.content = htmlMsg.c_str();
  message.text.charSet = "us-ascii";
  message.html.transfer_encoding = Content_Transfer_Encoding::enc_7bit;

  message.priority = esp_mail_smtp_priority::esp_mail_smtp_priority_low;
  message.response.notify = esp_mail_smtp_notify_success | esp_mail_smtp_notify_failure | esp_mail_smtp_notify_delay;

  /* Connect to the server */
  if (!smtp.connect(&config))
  {
    ESP_MAIL_PRINTF("Connection error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
    return;
  }

  if (!smtp.isLoggedIn())
  {
    Serial.println("\nNot yet logged in.");
  }
  else
  {
    if (smtp.isAuthenticated())
      Serial.println("\nSuccessfully logged in.");
    else
      Serial.println("\nConnected with no Auth.");
  }

  /* Start sending Email and close the session */
  if (!MailClient.sendMail(&smtp, &message))
    ESP_MAIL_PRINTF("Error, Status Code: %d, Error Code: %d, Reason: %s", smtp.statusCode(), smtp.errorCode(), smtp.errorReason().c_str());
}

/* Callback function to get the Email sending status */
void smtpCallback(SMTP_Status status)
{
  /* Print the current status */
  Serial.println(status.info());

  /* Print the sending result */
  if (status.success())
  {

    Serial.println("----------------");
    ESP_MAIL_PRINTF("Message sent success: %d\n", status.completedCount());
    ESP_MAIL_PRINTF("Message sent failed: %d\n", status.failedCount());
    Serial.println("----------------\n");

    for (msgNo = 0; msgNo < smtp.sendingResult.size(); msgNo++)
    {
      /* Get the result item */
      SMTP_Result result = smtp.sendingResult.getItem(msgNo);

      // In case, ESP32, ESP8266 and SAMD device, the timestamp get from result.timestamp should be valid if
      // your device time was synched with NTP server.
      // Other devices may show invalid timestamp as the device time was not set i.e. it will show Jan 1, 1970.
      // You can call smtp.setSystemTime(xxx) to set device time manually. Where xxx is timestamp (seconds since Jan 1, 1970)

      ESP_MAIL_PRINTF("Message No: %d\n", msgNo + 1);
      ESP_MAIL_PRINTF("Status: %s\n", result.completed ? "success" : "failed");
      ESP_MAIL_PRINTF("Date/Time: %s\n", MailClient.Time.getDateTimeString(result.timestamp, "%B %d, %Y %H:%M:%S").c_str());
      ESP_MAIL_PRINTF("Recipient: %s\n", result.recipients.c_str());
      ESP_MAIL_PRINTF("Subject: %s\n", result.subject.c_str());
    }
    Serial.println("----------------\n");

    // You need to clear sending result as the memory usage will grow up.
    smtp.sendingResult.clear();
    // --------------------------- No Changes to the above function smtpCallback ----------------------
  }
}

String dateFormat(int yearVal, int monthVal, int dayVal)
{
  String dateVal;
  if ((dayVal < 10) || (monthVal < 10))
  {
    if (dayVal < 10)
    {
      dateVal = String("0") + String(dayVal);
    }
    else
    {
      dateVal = String(dayVal);
    }

    if (monthVal < 10)
    {
      dateVal += "." + String("0") + String(monthVal) + "." + String(yearVal);
    }
    else
    {
      dateVal += "." + String(monthVal) + "." + String(yearVal);
    }
  }
  else
  {
    dateVal = String(dayVal) + "." + String(monthVal) + "." + String(yearVal);
  }
  return dateVal;
}

String timeFormat(int hrVal, int minVal, int secVal)
{
  String timeVal;
  if (hrVal < 10 || minVal < 10 || secVal < 10)
  {
    if (hrVal < 10)
    {
      timeVal = String("0") + String(hrVal);
    }
    else
    {
      timeVal = String(hrVal);
    }

    if (minVal < 10)
    {
      timeVal += ":" + String("0") + String(minVal);
    }
    else
    {
      timeVal += ":" + String(minVal);
    }

    if (secVal < 10)
    {
      timeVal += ":" + String("0") + String(secVal);
    }
    else
    {
      timeVal += ":" + String(secVal);
    }
  }
  else
  {
    timeVal = String(hrVal) + ":" + String(minVal) + ":" + String(secVal);
  }
  return timeVal;
}