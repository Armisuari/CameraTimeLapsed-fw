#pragma once

#include <string>
#include <stdint.h>

class StorageInterface
{
public:
    virtual bool init() = 0;
    virtual bool writeFile(std::string jsonString) = 0;
    virtual bool writeNumCapture(std::string jsonString) = 0;
    virtual bool writeLastCommand(const char *jsonString) = 0;
    virtual std::string readFile() = 0;
    virtual std::string readNumCapture() = 0;
    virtual std::string readLastCommand() = 0;
    virtual bool deleteFileLastCommand() = 0;
};