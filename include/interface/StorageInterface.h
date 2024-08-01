#pragma once

#include <string>
#include <stdint.h>

class StorageInterface
{
public:
    virtual bool init() = 0;
    virtual bool writeFile(std::string jsonString) = 0;
    virtual std::string readFile() = 0;
};