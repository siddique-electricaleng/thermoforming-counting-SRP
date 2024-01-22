/* System libraries */
#include <Arduino.h>
#include <stdio.h>

/* My Libraries */
#include "myWiFiMulti.h"
#include "myDateTime.h"
#include "sdCardFileOperations.h"

/* Global Date Variables - declared in myDateTime.h */
int nowYear, nowMonth, nowDay;

/* Global Time Variables - declared in myDateTime.h */
int nowHr, nowMin, nowSec;

/* Local Date Variables */
String dateString;
/* Local Time Variables */
String todayTimeStart, bodyRecTimeStart, bodyRecTimeStop;

/* Contains all the initialization code */
void setup()
{
  /* Initialize onboard LED first then WiFi */
  pinMode(LED, OUTPUT);
  pinMode(SENSE, INPUT);
  initWiFi(); // takes no inputs

  /* Serial Monitor */
  Serial.begin(115200);

  /* Synchronize and configure the local time */
  setupTime();
  // Get all date and time variables
  nowAllTimeData(&nowYear, &nowMonth, &nowDay, &nowHr, &nowMin, &nowSec);
}

void loop()
{
  delay(1000);
  setupTime();                                                            // updates the timeDate variable
  nowAllTimeData(&nowYear, &nowMonth, &nowDay, &nowHr, &nowMin, &nowSec); // use updated timeData to store it in these variables
  Serial.print("Current Date is: ");
  Serial.println(dateFormat(nowYear, nowMonth, nowDay));
  Serial.print("Current Time is: ");
  Serial.println(timeFormat(nowHr, nowMin, nowSec));
}