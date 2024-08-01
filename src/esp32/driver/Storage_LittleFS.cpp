#include "Storage_LittleFS.h"

bool Storage_LittleFS::init()
{
    if (!LittleFS.begin(true))
    {
        log_e("LITTLEFS Mount failed");
        return false;
    }
    else
    {
        log_i("SPIFFS mounted successfully");
    }

    return true;
}

bool Storage_LittleFS::writeFile(std::string jsonString)
{
    File file = LittleFS.open(filename.c_str(), "w");
    if (!file)
    {
        log_e("writeFile -> failed to open file for writing");
        return false;
    }
    if (file.print(jsonString.c_str()))
    {
        log_i("New config file written");
    }
    else
    {
        log_e("Write failed");
    }
    file.close();
    return true;
}

std::string Storage_LittleFS::readFile()
{
    File file = LittleFS.open(filename.c_str());
    if (!file)
    {
        log_e("Failed to open config file for reading");
        return "";
    }

    std::string fileText = "";
    while (file.available())
    {
        fileText = file.readString().c_str();
    }
    file.close();
    return fileText;
}