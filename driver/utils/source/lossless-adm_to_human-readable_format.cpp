#include "driver/utils/include/lossless-adm_to_human-readable_format.h"
#include "driver/utils/include/date.h"
using namespace date;

std::string convertDaysSinceEpochToDateString(long days) {
    auto time = sys_days{date::January/1/1970} + date::days{days};

    auto ymd = year_month_day{time};
    auto year = static_cast<int>(ymd.year());
    auto month = static_cast<unsigned int>(ymd.month());
    auto day = static_cast<unsigned int>(ymd.day());

    char dateString[11];
    std::sprintf(dateString, "%04d-%02u-%02u", year, month, day);

    return std::string(dateString);
}
std::string convertMillisecondsSinceBeginningOfDayToTimeString(long long milliseconds) {
    // Convert milliseconds to duration
    std::chrono::milliseconds duration(milliseconds);

    // Extract the time components
    auto timeComponents = hh_mm_ss(duration);

    // Format the time string
    char buffer[9];
    std::sprintf(buffer, "%02d:%02d:%02d", timeComponents.hours().count(), timeComponents.minutes().count(), timeComponents.seconds().count());

    return std::string(buffer);
}
std::string convertMillisecondsSinceEpochToDateTimeString(long long milliseconds) {
    // Convert milliseconds to duration
    std::chrono::milliseconds duration(milliseconds);

    // Create a time_point representing the given milliseconds
    auto tp = sys_time<system_clock::duration>{duration};

    // Define the time zone offset
    auto timeZoneOffset = 0h;  // UTC offset

    // Adjust the time point by adding the time zone offset
    auto adjustedTimePoint = tp + timeZoneOffset;

    // Format the adjusted time point as a local time string
    auto dateTimeString = format("%Y-%m-%d %H:%M:%S", adjustedTimePoint);
    return dateTimeString.substr(0,23);
}