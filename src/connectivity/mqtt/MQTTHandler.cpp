#include "MQTTHandler.h"
#include "MQTTHandlerDefinitions.h"
#include "WifiHandler.h"
#include "CONFIG.h"

WifiHandler _wifi(CONFIG_MAIN_WIFI_DEFAULT_SSID, CONFIG_MAIN_WIFI_DEFAULT_PASS);

MQTTHandler::MQTTHandler()
{
    client.setClient(espClient);
}

void MQTTHandler::setup(const char *addr, const uint16_t port)
{
    setup(addr, port, NULL);
}

void MQTTHandler::setup(const char *addr, const uint16_t port, const char *clientID)
{
    _wifi.init();

    if (clientID != NULL)
    {
        _clientID = clientID;
    }

    // create task for this handler
    xTaskCreate(&MQTTHandler::_staticTaskFunc,
                CONFIG_MQTT_HANDLER_TASK_NAME,
                CONFIG_MQTT_HANDLER_TASK_STACK,
                this,
                CONFIG_MQTT_HANDLER_TASK_PRIO,
                &_taskHandle);

    // Running the MQTT server
    client.setServer(addr, port);
    client.setCallback(std::bind(&MQTTHandler::MqttReceiveCallback, this,
                                 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
    client.setBufferSize(2048);
}

/* STATIC */ void MQTTHandler::_staticTaskFunc(void *pvParam)
{
    MQTTHandler *handler = reinterpret_cast<MQTTHandler *>(pvParam);
    handler->_taskFunc();
}

void MQTTHandler::_taskFunc()
{
    ESP_LOGD(MQTTHANDLERTAG, "MQTT Handler Task Started");

    while (1)
    {
        // if(!WifiInfo::isConnected())
        if (!WiFi.isConnected())
        {
            ESP_LOGD(MQTTHANDLERTAG, "Waiting wifi connection...");
            client.disconnect();

            // while (!WifiInfo::isConnected()) {
            while (!WiFi.isConnected())
            {
                // TODO: find mechanism to block task and immediately resume task if wifi connected
                delay(1000);
            }

            ESP_LOGD(MQTTHANDLERTAG, "Wifi connected, IP Address: %s",
                     // IPAddress(WifiInfo::localIP().addr).tostd::string().c_str());
                     WiFi.localIP().toString().c_str());
        }
        // else if(WifiInfo::isConnected() && !client.connected())
        else if (WiFi.isConnected() && !client.connected())
        {
            ESP_LOGD(MQTTHANDLERTAG, "Connecting to MQTT broker...");
            client.connect(_clientID.c_str());
        }
        else
        {
            // need to simplify into only one topic for remote command
            client.subscribe("reqStream");
            client.subscribe("angkasaConfig");
            client.subscribe("angkasa/deviceSetConfig");
            client.subscribe("angkasa/devicePower");

            client.loop();

            uint32_t newPubAvail = ulTaskNotifyTake(pdFALSE, pdMS_TO_TICKS(1000));
            if (newPubAvail > 0)
            {
                for (auto pubsource : _publishSources)
                {
                    if (!pubsource->available())
                    {
                        continue;
                    }

                    std::string payload = pubsource->readMQTTPayload();
                    if (payload.length() == 0)
                    {
                        continue;
                    }

                    std::string topic = pubsource->getTopic();
                    bool res = client.publish(topic.c_str(), payload.c_str());
                    ESP_LOGD(MQTTHANDLERTAG, "Publishing: topic: %s | msg: %s", topic.c_str(), payload.c_str());
                    if (!res)
                    {
                        ESP_LOGE(MQTTHANDLERTAG, "Publish Fail");
                        break;
                    }
                }
            }
        }
    }
    vTaskDelete(NULL);
}

bool MQTTHandler::addPublishSource(MQTTPublishSource *pubSource)
{
    if (!pubSource)
    {
        return false;
    }

    for (size_t i = 0; i < _publishSources.size(); i++)
    {
        if (_publishSources[i] == pubSource)
        {
            return false;
        }
    }

    if (!pubSource->init(_taskHandle))
    {
        return false;
    }
    _publishSources.push_back(pubSource);

    return true;
}

void MQTTHandler::MqttReceiveCallback(char *topic, byte *message, unsigned int length)
{
    std::string messageStr((char *)message, length);
    Serial.printf("Message arrived on topic: [%s]. Message: %s\n", topic, messageStr.c_str());
    _remoteMessage = std::move(messageStr);
    messageStr.clear();
}

bool MQTTHandler::getRemoteMsg(std::string &remoteMessage)
{
    if (_remoteMessage.empty())
    {
        return false;
    }

    remoteMessage = std::move(_remoteMessage);
    return true;
}