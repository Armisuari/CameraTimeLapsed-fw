#pragma once

#include <interface/SwitchPowerInterface.h>
#include <Arduino.h>

class Switch_Mosfet : public SwitchPowerInterface
{
public:
    Switch_Mosfet(uint8_t pin);
    void init();
    void on();
    void off();
    void oneCycleOn();

private:
    uint8_t _pin;
};