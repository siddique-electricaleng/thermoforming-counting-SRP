#ifndef myDateTime_H
#define myDateTime_H

#include <Arduino.h>
#include <stdio.h>
#include "time.h"

/* Setup function prototypes */
void setupTime();
/* Both Setup and Loop function prototypes */
void nowAllTimeData(int *currentYear, int *currentMonth, int *currentDay, int *currentHr, int *currentMin, int *currentSec);
/* Loop function prototypes */
String dateFormat(int yearVal, int monthVal, int dayVal);
String timeFormat(int hrVal, int minVal, int secVal);

extern struct tm timeData;

/* The Date and Time variables */
extern int nowYear, nowMonth, nowDay;
extern int nowHr, nowMin, nowSec;

#endif