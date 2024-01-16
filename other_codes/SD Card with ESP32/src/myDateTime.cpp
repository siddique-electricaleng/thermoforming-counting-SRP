#include "myDateTime.h"

/* Definition of variables used in this source file only */
const long gmtOffset_sec = 6 * 3600;
const int daylightOffset_sec = 0;

// timeData variable to give the time and date from the ntp servers
struct tm timeData;

/* Setup Function */
void setupTime()
{
    configTime(gmtOffset_sec, daylightOffset_sec, "pool.ntp.org", "time.nist.gov");
    if (!getLocalTime(&timeData))
    {
        Serial.println("Failed to obtain time. Trying to obtain it ");
        delay(500);
        while (!getLocalTime(&timeData))
        {
            delay(500);
            Serial.print(".");
        }
        return;
    }
}

/* Functions in both Setup and Loop */

/* nowAllTimeData() Function description
nowAllTimeData() function: retrieves the latest date and time.
Input: None
Output: All are integers - individual Date Variables(Year, Month, Day) and Individual Time Variables(Hour, Minute, Sec) */
void nowAllTimeData(int *currentYear, int *currentMonth, int *currentDay, int *currentHr, int *currentMin, int *currentSec)
{
    /* Date Acquisition */
    *currentYear = timeData.tm_year;
    *currentMonth = timeData.tm_mon + 1;
    *currentDay = timeData.tm_mday + 1900;

    /* Time Acquisition */
    *currentHr = timeData.tm_hour;
    *currentMin = timeData.tm_min;
    *currentSec = timeData.tm_sec;
}

/* Loop Functions */
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