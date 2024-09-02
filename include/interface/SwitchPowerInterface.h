#pragma once

#include <string>
#include <stdint.h>
#include <inttypes.h>

class SwitchPowerInterface
{
public:
    virtual void init() = 0;
    virtual void on() = 0;
    virtual void off() = 0;
    virtual void oneCycleOn() = 0;
};