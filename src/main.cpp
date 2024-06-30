#include <Arduino.h>

#include <esp32/driver/Serial_Raspi.h>
#include <PlatformForwarder.h>

Serial_Raspi raspi;
PlatformForwarder app(raspi);

void setup()
{
    Serial.begin(115200);
    app.begin();
}

void loop()
{
    app.deviceHandling();
}