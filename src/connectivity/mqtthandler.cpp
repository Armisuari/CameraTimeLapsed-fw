#include "mqtthandler.h"

HardwareSerial MySerial(1);

WifiHandler _wifi(CONFIG_MAIN_WIFI_DEFAULT_SSID, CONFIG_MAIN_WIFI_DEFAULT_PASS);

MQTTHandler *MQTTHandler::instance = nullptr; // Initialize the static instance pointer

MQTTHandler::MQTTHandler(const char *ssid, const char *password, const char *mqtt_server, int mqtt_port)
    : ssid(ssid), password(password), mqtt_server(mqtt_server), mqtt_port(mqtt_port), client(espClient), _running(true)
{
    instance = this; // Set the static instance pointer to this instance
}

void MQTTHandler::reconnect()
{
    while (!client.connected())
    {
        Serial.print("Attempting MQTT connection...");
        clientId += String(random(0xffff), HEX);
        if (client.connect(clientId.c_str()))
        {
            Serial.println("connected");
            client.subscribe("reqStream");
        }
        else
        {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            delay(5000);
        }
    }
}

void MQTTHandler::callback(char *topic, byte *payload, unsigned int length)
{
    if (instance != nullptr)
    {
        instance->handleCallback(topic, payload, length);
    }
}

void MQTTHandler::handleCallback(char *topic, byte *payload, unsigned int length)
{
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
    for (int i = 0; i < length; i++)
    {
        Serial.print((char)payload[i]);
    }
    Serial.println();

    _message = "";

    // Convert payload to string
    for (int i = 0; i < length; i++)
    {
        _message += (char)payload[i];
    }
}

// bool MQTTHandler::processMessage(const char *message)
bool MQTTHandler::processMessage(std::string &message)
{
    static std::string lastMsg;

    // static uint64_t lastUpdate;
    // if (millis() - lastUpdate >= 1000U)
    // {
    //     lastUpdate = millis();
    //     log_d("lastMsg = %s", lastMsg.c_str());
    // }

    if (_message == lastMsg)
    {
        return false;
    }

    log_i("data changed");
    message = _message;
    lastMsg = _message;

    return true;
}

void MQTTHandler::init()
{
    _wifi.init();
    client.setServer(mqtt_server, mqtt_port);
    // client.setCallback(callback);
    client.setCallback(MQTTHandler::callback);

    // _taskThread = std::thread(&MQTTHandler::taskFunc, this);
    xTaskCreate(taskFunc, "taskFunc", 1024 * 4, this, 1, NULL);
}

// void MQTTHandler::loop()
// {
//     // this->taskFunc();
//     if (!client.connected())
//     {
//         reconnect();
//     }
//     client.loop();
// }

bool MQTTHandler::publish(std::string message)
{
    if (WiFi.status() != WL_CONNECTED)
    {
        log_w("waiting wifi connection");
        client.disconnect();

        while (WiFi.status() != WL_CONNECTED)
        {
            delay(1000);
        }
    }
    else if (WiFi.status() == WL_CONNECTED && !client.connected())
    {
        log_i("connectiong to platform...");
        if (!client.connect(clientId.c_str()))
        {
            log_e("failed to connect to platform !");
            return false;
        }
    }

    bool res = client.publish("settings", message.c_str());

    if (!res)
    {
        log_e("Failed to publish message");
        return false;
    }

    return true;
}

// void MQTTHandler::taskFunc()
// {
//     while (_running)
//     {
//         if (!client.connected())
//         {
//             reconnect();
//         }
//         client.loop();
//     }
// }
void MQTTHandler::taskFunc(void *pvParam)
{
    MQTTHandler *app = static_cast<MQTTHandler *>(pvParam);
    while (true)
    {
        if (!app->client.connected())
        {
            app->reconnect();
        }
        app->client.loop();
    }
}
