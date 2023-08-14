#include <iostream>
#include <iomanip>
#include <chrono>
#include <string>
using namespace std::chrono;

/**
* The lossless-adm format is used to preserve precision when dealing with double data type.
* <p/>
* As a result, retrieved values from the CBAS server for date, time, and datetime datatypes are
* in the lossless-ADM format. The functions that follow convert these values into a human-readable format.
* <p/>
* https://github.com/couchbase/asterixdb/blob/fc0cbd0f5b0002a3fc4b15820143af56c88616d6/asterixdb/asterix-om/src/main/java/org/apache/asterix/formats/nontagged/LosslessADMJSONPrinterFactoryProvider.java#L76
* <p/>
* The date.h library used is: https://github.com/HowardHinnant/date/blob/master/include/date/date.h
* <p/>
*/

/**
    Converts number of full calendar days since 1970-01-01 to YYYY:MM:DD Date value.
    @param[in] days Integer value representing days since epoch.
    \return         String value reprsenting date
*/
std::string convertDaysSinceEpochToDateString(long int days);

/**
    Converts number of milliseconds since beginning of the day to hh:mm:ss Time format.
    @param[in] days Integer value representing milliseconds since since beginning of the day.
    \return         String value reprsenting time
*/
std::string convertMillisecondsSinceBeginningOfDayToTimeString(long long int);

/**
    Converts number of milliseconds since 1970-01-01T00:00:00.000Z. to YYYY-MM-DD hh:mm:ss.s Datetime format.
    @param[in] days Integer value representing milliseconds since epoch.
    \return         String value reprsenting datetime
*/
std::string convertMillisecondsSinceEpochToDateTimeString(long long int);