#include "CaptureScheduler.h"
#include "CaptureSchedulerDef.h"
#include <ArduinoJson.h>

// Static instance initialization.
CaptureScheduleHandler::TimeCaptureData_t CaptureScheduleHandler::_timeCaptureData = {};

CaptureScheduleHandler::CaptureScheduleHandler(fs::FS &fs) : _fs(fs) {}

bool CaptureScheduleHandler::begin()
{
    // Attempt to load the saved schedule data from the file system.
    if (!load())
    {
        ESP_LOGE(CAPTUREHANDLERTAG, "Failed to load fs, create new one");
        return save();
    }

    return true;
}

bool CaptureScheduleHandler::setTimeCapture(const std::string &captureTime)
{
    // Example: Validate the input string format (pseudo-validation here).
    if (captureTime.empty() || captureTime.length() != 8) // Example format: "HH:mm:ss"
    {
        return false;
    }

    // Convert and update the internal time data if needed (logic omitted for brevity).
    // Example: Update `_timeCaptureData` members from parsed `captureTime`.

    return save(); // Save the updated schedule.
}

std::string CaptureScheduleHandler::getTimeCapture() const
{
    // Generate a string representation of the internal capture time.
    char buffer[20];
    snprintf(buffer, sizeof(buffer), "%02d:%02d - %02d:%02d",
             _timeCaptureData.startHour, 0, _timeCaptureData.stopHour, 0);
    return std::string(buffer);
}

bool CaptureScheduleHandler::trigCapture()
{
    // Example logic to determine if capture should be triggered.
    time_t now = std::time(nullptr);
    tm *localTime = std::localtime(&now);

    uint8_t currentHour = localTime->tm_hour;
    if (currentHour >= _timeCaptureData.startHour && currentHour <= _timeCaptureData.stopHour)
    {
        // Capture condition met.
        return true;
    }
    return false;
}

uint16_t CaptureScheduleHandler::calcInterval()
{
    if (_timeCaptureData.stopHour < _timeCaptureData.startHour)
    {
        return 
    }
}

bool CaptureScheduleHandler::save()
{
    File file = _fs.open(_filename, FILE_WRITE);
    if (!file)
    {
        return false;
    }

    // Write the capture data to the file.
    file.write(reinterpret_cast<uint8_t *>(&_timeCaptureData), sizeof(_timeCaptureData));
    file.close();
    return true;
}

bool CaptureScheduleHandler::load()
{
    File file = _fs.open(_filename, FILE_READ);
    if (!file)
    {
        return false;
    }

    // Read the saved capture data from the file.
    if (file.read(reinterpret_cast<uint8_t *>(&_timeCaptureData), sizeof(_timeCaptureData)) != sizeof(_timeCaptureData))
    {
        file.close();
        return false;
    }

    file.close();
    return true;
}
