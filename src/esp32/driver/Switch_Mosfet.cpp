#include "Switch_Mosfet.h"

Switch_Mosfet::Switch_Mosfet(uint8_t pin) : _pin(pin)
{
}

void Switch_Mosfet::init()
{
    pinMode(_pin, OUTPUT);
}

void Switch_Mosfet::on()
{
    digitalWrite(_pin, HIGH);
}

void Switch_Mosfet::off()
{
    digitalWrite(_pin, LOW);
}

void Switch_Mosfet::oneCycleOn()
{
    off();
    delay(1000);
    on();
}