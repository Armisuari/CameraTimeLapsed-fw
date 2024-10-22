#pragma once

#include <string>
#include <ctime>
#include <cstdint>
#include <FS.h>

class CaptureScheduleHandler
{
public:
    // Constructor that initializes the file system reference.
    CaptureScheduleHandler(fs::FS &fs);

    // Initializes the capture schedule handler (e.g., loading saved data).
    bool begin();

    // Sets the capture time string and updates the internal structure.
    bool setTimeCapture(const std::string &captureTime);

    // Returns the currently set capture time as a string.
    std::string getTimeCapture() const;

    // Triggers the capture based on the time conditions.
    bool trigCapture();

private:
    // Internal filename to store time capture data.
    static constexpr const char *_filename = "/capture_schedule.dat";

    // Reference to the file system object (e.g., SPIFFS or SD).
    fs::FS &_fs;

    // Struct to store time capture configuration.
    typedef struct
    {
        uint32_t startDate;
        uint32_t stopDate;
        uint8_t startHour;
        uint8_t stopHour;
        uint8_t captureDays;
    } TimeCaptureData_t;

    // Static instance of the capture data.
    static TimeCaptureData_t _timeCaptureData;

    // Calculate max number of capture
    uint16_t calcInterval();

    // Saves the current capture configuration to the file system.
    bool save();

    // Loads the capture configuration from the file system.
    bool load();
};
