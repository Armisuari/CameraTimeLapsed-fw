#pragma once

#include <PubSubClient.h>
#include "MQTTPublishSource.h"
#include "WiFiClient.h"
#include <vector>

typedef struct
{
    int16_t shutterData;
    int16_t isoData;
    int16_t awbData;
    int16_t evData;
}CameraConfig;

class MQTTHandler 
{
    public:
        MQTTHandler();
        void setup(const char* addr, const uint16_t port);
        void setup(const char* addr, const uint16_t port, const char* clientID);
        bool addPublishSource(MQTTPublishSource* pubSource);

    private:
        TaskHandle_t _taskHandle;
        std::vector<MQTTPublishSource*> _publishSources;
        WiFiClient espClient;
        PubSubClient client;
        std::string _clientID;
        std::string _message;

        static void _staticTaskFunc(void* pvParam);
        void _taskFunc();

        void MqttReceiveCallback(char *topic, byte *message, unsigned int length);
        static void heartBeatTask(void *pvParameter);
};