#include "MQTTPublishSource.h"

MQTTPublishSource::MQTTPublishSource(std::string topic, size_t bufferSize) :
_topic(topic), _messageBuffSize(bufferSize), _receivingTaskHandle(NULL)
{
}

bool MQTTPublishSource::init(TaskHandle_t receivingTaskHandle)
{
    _receivingTaskHandle = receivingTaskHandle;
    _messageBuffHandle = xMessageBufferCreate(_messageBuffSize);
    return _messageBuffHandle != NULL;
}

std::string MQTTPublishSource::getTopic()
{
    return _topic;
}

size_t MQTTPublishSource::writeRawData(void* buff, size_t length)
{
    size_t res = xMessageBufferSend(_messageBuffHandle, buff, length, 0);
    if(res == length)
    {
        xTaskNotifyGive(_receivingTaskHandle);
    }
    return res;
}

size_t MQTTPublishSource::readRawData(void* buff, size_t length)
{
    return xMessageBufferReceive(_messageBuffHandle, buff, length, 0);
}

bool MQTTPublishSource::available()
{
    return (xMessageBufferIsEmpty(_messageBuffHandle) == pdFALSE);
}