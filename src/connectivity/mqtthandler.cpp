#include "mqtthandler.h"

HardwareSerial MySerial(1);

WifiHandler _wifi(CONFIG_MAIN_WIFI_DEFAULT_SSID, CONFIG_MAIN_WIFI_DEFAULT_PASS);

// MQTTHandler *MQTTHandler::instance = nullptr; // Initialize the static instance pointer

static const char *TAG = "MqttHandler";

EventGroupHandle_t MQTTHandler::_eventGroup = NULL;

MQTTHandler::MQTTHandler(const char *ssid, const char *password, const char *mqtt_server, int mqtt_port)
    : ssid(ssid), password(password), mqtt_server(mqtt_server), mqtt_port(mqtt_port), client(espClient)
{
    // instance = this; // Set the static instance pointer to this instance
}

/* STATIC */ void MQTTHandler::heartBeatTask(void *pvParameter)
{
    vTaskDelay(2000);
    MQTTHandler *instance = reinterpret_cast<MQTTHandler *>(pvParameter);
    log_d("\n\nheartBeatTask start !");
    std::string jsonMsg;
    DynamicJsonDocument doc(512);

    while (1)
    {
        static uint64_t count;
        count += 1;
        doc["time"] = time(NULL);
        doc["count"] = count;
        serializeJson(doc, jsonMsg);

        EventBits_t uxBit = xEventGroupGetBits(_eventGroup);
        if (uxBit & EVT_MQTT_CONNECTED)
        {
            log_d("publishing heartbeat");
            instance->publish("angkasa/heartbeat", jsonMsg);
        }
        else
        {
            log_e("failed to publish heartbeat");
            log_w("EVT_MQTT_DISCONNECTED");
        }

        vTaskDelay(60000 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}

void MQTTHandler::reconnect()
{
    log_d("reconnecting mqtt");
    while (!client.connected())
    {
        xEventGroupClearBits(_eventGroup, EVT_MQTT_CONNECTED);
        xEventGroupSetBits(_eventGroup, EVT_MQTT_DISCONNECTED);
        if (WiFi.status() == WL_CONNECTED)
        {
            log_w("Attempting MQTT connection...");
            clientId += String(random(0xffff), HEX);
            if (client.connect(clientId.c_str()))
            {
                log_i("mqtt connected");
                // client.subscribe("angkasa/checkDevice");
                xEventGroupClearBits(_eventGroup, EVT_MQTT_DISCONNECTED);
                xEventGroupSetBits(_eventGroup, EVT_MQTT_CONNECTED);

                EventBits_t uxBits = xEventGroupGetBits(_eventGroup);
                if (uxBits & EVT_MQTT_CONNECTED)
                {
                    log_i("EVT_MQTT_CONNECTED");
                }

                client.subscribe("reqStream");
                client.subscribe("angkasaConfig");
                client.subscribe("angkasa/deviceSetConfig");

                log_d("publishing device ready...");
                client.publish("angkasa/checkDevice", "Device Ready !");
            }
            else
            {
                log_e("failed, rc= %d | try again in 5 seconds", client.state());
                // xEventGroupClearBits(_eventGroup, EVT_MQTT_CONNECTED);
                // xEventGroupSetBits(_eventGroup, EVT_MQTT_DISCONNECTED);
                delay(2000);
            }
        }
    }
}

// void MQTTHandler::callback(char *topic, byte *payload, unsigned int length)
// {
//     handleCallback(topic, payload, length);
// }

// void MQTTHandler::handleCallback(char *topic, byte *payload, unsigned int length)
void MQTTHandler::callback(char *topic, byte *payload, unsigned int length)
{
    Serial.printf("Message arrived [%s] ", topic);
    _message = std::move(std::string((char *)payload, length));
    Serial.println(_message.c_str());
}

bool MQTTHandler::processMessage(std::string &message)
{
    // log_d("%s", _message.c_str());
    static bool _loggedEmptyMessage = false;
    if (_message.empty())
    {
        if (!_loggedEmptyMessage) // Check if the empty log has already been printed
        {
            log_e("_message is empty");
            _loggedEmptyMessage = true; // Set the flag to prevent repeated logs
        }
        return false;
    }

    // Check if command is about changing config
    if (_message.find("Shutter") != std::string::npos)
    {
        log_i("parsing config json..");
        handleConfig(_message.c_str(), _message.length());
    }

    message = std::move(_message);
    _message.clear();

    return true;
}

void MQTTHandler::handleConfig(const char *config, unsigned int length)
{
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, config, length);
    if (error)
    {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    if (doc.containsKey("Shutter"))
    {
        const char *SHUTTER = doc["Shutter"];
        Serial.print("SHUTTER Data: ");
        Serial.println(SHUTTER);

        if (strcmp(SHUTTER, "AUTO") == 0)
        {
            shutterdata = 0;
            Serial.println("Data SHUTTER set to 0");
        }
        else if (strcmp(SHUTTER, "1/125") == 0)
        {
            shutterdata = 1;
            Serial.println("Data SHUTTER set to 1");
        }
        else if (strcmp(SHUTTER, "1/250") == 0)
        {
            shutterdata = 2;
            Serial.println("Data SHUTTER set to 2");
        }
        else if (strcmp(SHUTTER, "1/500") == 0)
        {
            shutterdata = 3;
            Serial.println("Data SHUTTER set to 3");
        }
        else if (strcmp(SHUTTER, "1/1000") == 0)
        {
            shutterdata = 4;
            Serial.println("Data SHUTTER set to 4");
        }
        else if (strcmp(SHUTTER, "1/2000") == 0)
        {
            shutterdata = 5;
            Serial.println("Data SHUTTER set to 5");
        }
        else
        {
            Serial.println("Failed to handle shutter data");
        }
    }

    if (doc.containsKey("ISO"))
    {
        const char *iso = doc["ISO"];
        Serial.print("iso Data: ");
        Serial.println(iso);

        if (strcmp(iso, "800") == 0)
        {
            isodata = 0;
            Serial.println("Data iso set to 0");
        }

        else if (strcmp(iso, "400") == 0)
        {
            isodata = 1;
            Serial.println("Data iso set to 1");
        }

        else if (strcmp(iso, "200") == 0)
        {
            isodata = 2;
            Serial.println("Data iso set to 2");
        }

        else if (strcmp(iso, "100") == 0)
        {
            isodata = 3;
            Serial.println("Data iso set to 3");
        }

        else if (strcmp(iso, "1600") == 0)
        {
            isodata = 4;
            Serial.println("Data iso set to 4");
        }
        else if (strcmp(iso, "3200") == 0)
        {
            isodata = 5;
            Serial.println("Data iso set to 5");
        }
        else
        {
            Serial.println("Failed to handle iso data");
        }
    }

    if (doc.containsKey("AWB"))
    {
        const char *awb = doc["AWB"];
        Serial.print("awb Data: ");
        Serial.println(awb);

        if (strcmp(awb, "AUTO") == 0)
        {
            awbdata = 0;
            Serial.println("Data awb set to 0");
        }

        else if (strcmp(awb, "5500K") == 0)
        {
            awbdata = 2;
            Serial.println("Data awb set to 2");
        }

        else if (strcmp(awb, "6500K") == 0)
        {
            awbdata = 3;
            Serial.println("Data awb set to 3");
        }

        else if (strcmp(awb, "NATIVE") == 0)
        {
            awbdata = 4;
            Serial.println("Data awb set to 4");
        }

        else if (strcmp(awb, "4000K") == 0)
        {
            awbdata = 5;
            Serial.println("Data awb set to 5");
        }

        else if (strcmp(awb, "6000K") == 0)
        {
            awbdata = 7;
            Serial.println("Data awb set to 7");
        }

        else if (strcmp(awb, "2300K") == 0)
        {
            awbdata = 8;
            Serial.println("Data awb set to 8");
        }

        else if (strcmp(awb, "2800K") == 0)
        {
            awbdata = 9;
            Serial.println("Data awb set to 9");
        }

        else if (strcmp(awb, "3200K") == 0)
        {
            awbdata = 10;
            Serial.println("Data awb set to 10");
        }

        else if (strcmp(awb, "4500K") == 0)
        {
            awbdata = 11;
            Serial.println("Data awb set to 11");
        }

        else if (strcmp(awb, "5000K") == 0)
        {
            awbdata = 12;
            Serial.println("Data awb set to 12");
        }

        else
        {
            Serial.println("Failed to handle awb data");
        }
    }

    if (doc.containsKey("EVCOMP"))
    {
        const char *ev = doc["EVCOMP"];
        Serial.print("ev Data: ");
        Serial.println(ev);

        if (strcmp(ev, "2.0") == 0)
        {
            evdata = 0;
            Serial.println("Data ev set to 0");
        }

        else if (strcmp(ev, "1.5") == 0)
        {
            evdata = 1;
            Serial.println("Data ev set to 1");
        }

        else if (strcmp(ev, "1.0") == 0)
        {
            evdata = 2;
            Serial.println("Data ev set to 2");
        }

        else if (strcmp(ev, "0.5") == 0)
        {
            evdata = 3;
            Serial.println("Data ev set to 3");
        }

        else if (strcmp(ev, "0.0") == 0)
        {
            evdata = 4;
            Serial.println("Data ev set to 4");
        }

        else if (strcmp(ev, "-0.5") == 0)
        {
            evdata = 5;
            Serial.println("Data ev set to 5");
        }

        else if (strcmp(ev, "-1.0") == 0)
        {
            evdata = 6;
            Serial.println("Data ev set to 6");
        }

        else if (strcmp(ev, "-1.5") == 0)
        {
            evdata = 7;
            Serial.println("Data ev set to 7");
        }

        else if (strcmp(ev, "-2.0") == 0)
        {
            evdata = 8;
            Serial.println("Data ev set to 8");
        }

        else
        {
            Serial.println("Failed to handle ev data");
        }
    }

    if (doc.containsKey("ScheduleEnabled"))
    {
        const char *isSchedule = doc["ScheduleEnabled"];
        Serial.print("Schedule Data: ");
        Serial.println(isSchedule);
        if (strcmp(isSchedule, "1") == 0)
        {
            isScheduleOn = true;
            Serial.println("Scheduled capture enabled");
        }
        else
        {
            isScheduleOn = false;
            Serial.println("Scheduled capture disabled");
        }
    }

    _message = "{" + std::string("\"shu\":") + std::to_string(shutterdata) + "," +
               "\"iso\":" + std::to_string(isodata) + "," +
               "\"wb\":" + std::to_string(awbdata) + "," +
               "\"ev\":" + std::to_string(evdata) + "}";

    // Serial.println(_message.c_str());
}

void MQTTHandler::init()
{
    _wifi.init();
    client.setServer(mqtt_server, mqtt_port);
    // client.setCallback(MQTTHandler::callback);
    // client.setCallback(callback);
    client.setCallback(std::bind(&MQTTHandler::callback, this,
                                 std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

    _eventGroup = xEventGroupCreate();
    if (_eventGroup == NULL)
    {
        log_e("Failed to create eventGroup");
        return;
    }
    // xEventGroupClearBits(_eventGroup, EVT_MQTT_CONNECTED);
    xEventGroupSetBits(_eventGroup, EVT_MQTT_DISCONNECTED);

    xTaskCreate(&MQTTHandler::taskFunc, "taskFunc", 1024 * 4, this, 1, NULL);
    xTaskCreate(&MQTTHandler::heartBeatTask, "heartbeat", 4096, this, 1, NULL);
}

bool MQTTHandler::publish(std::string topic, std::string message)
{
    log_d("publishing message...");
    if (WiFi.status() != WL_CONNECTED)
    {
        ESP_LOGW(TAG, "waiting wifi connection");
        client.disconnect();

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(1000);
        }
    }
    else if (WiFi.status() == WL_CONNECTED && !client.connected())
    {
        ESP_LOGD(TAG, "connecting to platform...");
        if (!client.connect(clientId.c_str()))
        {
            ESP_LOGE(TAG, "failed to connect to platform !");
            return false;
        }
    }

    EventBits_t uxBits = xEventGroupGetBits(_eventGroup);
    if (uxBits & EVT_MQTT_CONNECTED)
    {
        log_i("EVT_MQTT_CONNECTED");
        // xEventGroupWaitBits(_eventGroup, EVT_MQTT_CONNECTED, pdTRUE, pdFALSE, portMAX_DELAY);
        bool res = client.publish(topic.c_str(), message.c_str());

        if (!res)
        {
            ESP_LOGE(TAG, "Failed to publish message");
            return false;
        }
        else
        {
            ESP_LOGD(TAG, "Succed to publish message : %s", message.c_str());
        }
    }
    else
    {
        log_w("EVT_MQTT_DISCONNECTED");
        return false;
    }

    return true;
}

void MQTTHandler::taskFunc(void *pvParam)
{
    log_d("start mqtt task");
    MQTTHandler *instance = reinterpret_cast<MQTTHandler *>(pvParam);
    while (true)
    {
        if (!instance->client.connected())
        {
            instance->reconnect();
        }
        instance->client.loop();
    }
}

bool MQTTHandler::isScheduleEnabled()
{
    return isScheduleOn;
}
