#pragma once

#include <LittleFS.h>
#include <interface/StorageInterface.h>

class Storage_LittleFS : public StorageInterface
{
public:
    bool init();
    bool writeFile(std::string jsonString);
    bool writeNumCapture(std::string jsonString);
    bool writeLastCommand(const char* jsonString);
    std::string readFile();
    std::string readNumCapture();
    std::string readLastCommand();
    bool deleteFileLastCommand();

private:
    std::string filename = "/camera_config.json";
    std::string fileNumCapture = "/capture_number.json";
    std::string fileLastCommand = "/last_command.json";
};