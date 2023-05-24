#include "driver/utils/lossless-adm_to_human-readable_format.h"

std::string convertEpochDaysToDateString(long days) {
    auto epoch = std::chrono::system_clock::from_time_t(0);
    auto time = epoch + std::chrono::hours(24) * days;
    std::time_t currentTime = std::chrono::system_clock::to_time_t(time);
    std::tm* timeInfo = std::gmtime(&currentTime);

    long year = timeInfo->tm_year + 1900;
    long month = timeInfo->tm_mon + 1;
    long day = timeInfo->tm_mday;

    char dateString[11];
    std::sprintf(dateString, "%04ld:%02ld:%02ld", year, month, day);

    return std::string(dateString);
}

std::string convertEpochMillisecondsToTimeString(long long milliseconds) {
    auto duration = std::chrono::milliseconds(milliseconds);

    auto hours = std::chrono::duration_cast<std::chrono::hours>(duration);
    duration -= hours;

    auto minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
    duration -= minutes;

    auto seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
    duration -= seconds;

    auto millisecondsRemain = duration.count();

    std::string formattedTime;
    formattedTime += std::to_string(hours.count() / 10);
    formattedTime += std::to_string(hours.count() % 10);
    formattedTime += ":";
    formattedTime += std::to_string(minutes.count() / 10);
    formattedTime += std::to_string(minutes.count() % 10);
    formattedTime += ":";
    formattedTime += std::to_string(seconds.count() / 10);
    formattedTime += std::to_string(seconds.count() % 10);

    return formattedTime;
}

std::string convertEpochMillisecondsToDateTimeString(long long milliseconds) {
    // Convert epoch milliseconds to std::chrono::system_clock::time_point
    auto timePoint = std::chrono::time_point<std::chrono::system_clock>(
        std::chrono::milliseconds(milliseconds)
    );

    // Convert std::chrono::system_clock::time_point to std::tm
    std::time_t currentTime = std::chrono::system_clock::to_time_t(timePoint);
    std::tm* timeInfo = std::gmtime(&currentTime);

    // Format the date and time string
    char buffer[24];
    std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", timeInfo);

    // Get the remaining milliseconds
    auto duration = timePoint.time_since_epoch();
    auto millisecondsRemain = std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() % 1000;

    // Combine the formatted date and time string with milliseconds
    char formattedDateTime[28];
    std::snprintf(formattedDateTime, sizeof(formattedDateTime), "%s.%03lld", buffer, millisecondsRemain);

    return std::string(formattedDateTime);
}