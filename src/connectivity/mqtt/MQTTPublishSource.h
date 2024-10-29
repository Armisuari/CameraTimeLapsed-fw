#pragma once

#include <string>
#include "freertos/FreeRTOS.h"
#include "freertos/message_buffer.h"
#include "freertos/task.h"

class MQTTPublishSource
{
    friend class MQTTHandler;
public:
    MQTTPublishSource(std::string topic, size_t bufferSize);
    std::string getTopic();
    size_t writeRawData(void* buff, size_t length);

protected:
    bool init(TaskHandle_t receivingTaskHandle);
    size_t readRawData(void* buff, size_t length);
    bool available();
    virtual std::string readMQTTPayload() = 0;
    virtual size_t readMQTTPayload(uint8_t* buff, size_t length) = 0;

private:
    std::string _topic;
    MessageBufferHandle_t _messageBuffHandle;
    size_t _messageBuffSize;
    TaskHandle_t _receivingTaskHandle;
};