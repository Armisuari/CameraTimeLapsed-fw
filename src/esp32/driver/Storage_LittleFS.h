#pragma once

#include <LittleFS.h>
#include <interface/StorageInterface.h>

class Storage_LittleFS : public StorageInterface
{
public:
    bool init();
    bool writeFile(std::string jsonString);
    std::string readFile();

private:
    std::string filename = "/camera_config.json";
};