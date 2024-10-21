#include "Storage_LittleFS.h"

bool Storage_LittleFS::init()
{
    if (!LittleFS.begin())
    {
        log_e("LITTLEFS Mount failed");
        return false;
    }
    else
    {
        log_i("LITTLEFS mounted successfully");
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

bool Storage_LittleFS::writeNumCapture(std::string jsonString)
{
    File file = LittleFS.open(fileNumCapture.c_str(), "w");
    if (!file)
    {
        log_e("writeFile -> failed to open file for writing -> %s", fileNumCapture.c_str());
        return false;
    }
    if (file.print(jsonString.c_str()))
    {
        log_i("number capture write succes");
    }
    else
    {
        log_e("Write failed");
    }
    file.close();
    return true;
}

bool Storage_LittleFS::writeLastCommand(const char *jsonString)
{
    File file = LittleFS.open(fileLastCommand.c_str(), "w");
    if (!file)
    {
        log_e("writeFile -> failed to open file for writing -> %s", fileLastCommand.c_str());
        return false;
    }
    if (file.print(jsonString))
    {
        log_i("number capture write succes");
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

std::string Storage_LittleFS::readNumCapture()
{
    File file = LittleFS.open(fileNumCapture.c_str(), "r");
    if (!file)
    {
        log_e("Failed to open file for reading -> %s", fileNumCapture.c_str());
        log_d("Creating file...");

        // Create the file with an initial empty content
        File newFile = LittleFS.open(fileNumCapture.c_str(), "w");
        if (!newFile)
        {
            log_e("Failed to create file -> %s", fileNumCapture.c_str());
            return "";
        }
        newFile.close();
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

std::string Storage_LittleFS::readLastCommand()
{
    File file = LittleFS.open(fileLastCommand.c_str(), "r");
    if (!file)
    {
        log_e("Failed to open file for reading -> %s", fileLastCommand.c_str());
        log_d("Creating file...");

        // Create the file with an initial empty content
        File newFile = LittleFS.open(fileLastCommand.c_str(), "w");
        if (!newFile)
        {
            log_e("Failed to create file -> %s", fileLastCommand.c_str());
            return "";
        }
        newFile.close();
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

bool Storage_LittleFS::deleteFileLastCommand()
{
    if (LittleFS.exists(fileLastCommand.c_str()))
    {
        if (LittleFS.remove(fileLastCommand.c_str()))
        {
            log_i("File deleted successfully -> %s", fileLastCommand.c_str());
            return true;
        }
        else
        {
            log_e("Failed to delete file -> %s", fileLastCommand.c_str());
            return false;
        }
    }
    else
    {
        log_d("File does not exist -> %s", fileLastCommand.c_str());
        return false;
    }
}
