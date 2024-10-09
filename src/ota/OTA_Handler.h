/*
 * OTAWebUpdater.ino Example from ArduinoOTA Library
 * Rui Santos 
 * Complete Project Details https://randomnerdtutorials.com
 * https://randomnerdtutorials.com/esp32-over-the-air-ota-programming/
 */
#pragma once

#include <WebServer.h>
// #include "config.h"

// #define CONFIG_MAIN_FW_VERSION_STRING "v1.0.2"

typedef std::function<void()> onOtaStateChange_t;

class OtaHandler 
{
    public:
        void setup(onOtaStateChange_t onStart = nullptr, onOtaStateChange_t onFinish = nullptr);
        static void task(void*);
        WebServer * getServer();  

    private:
        WebServer * server;   
        onOtaStateChange_t _onStartCb;
        onOtaStateChange_t _onFinishedCb;
};

extern OtaHandler otaHandler;
