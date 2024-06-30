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
        clientId += std::string(random(0xffff), HEX);
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

    // Convert payload to string
    for (int i = 0; i < length; i++)
    {
        _message += (char)payload[i];
    }

    // Now payloadStr contains the payload data as a string
    Serial.println(_message.c_str());

    // DynamicJsonDocument doc(1024);
    // DeserializationError error = deserializeJson(doc, payload, length);
    // if (error)
    // {
    //     Serial.print("deserializeJson() failed: ");
    //     Serial.println(error.c_str());
    //     return;
    // }

    // if (doc.containsKey("stream"))
    // {
    //     const char *message = doc["stream"];
    //     Serial.print("Data: ");
    //     Serial.println(message);

    //     // Call the other function with the extracted message
    //     bool success = processMessage(message);
    //     if (success)
    //     {
    //         Serial.println("Message processed successfully.");
    //     }
    //     else
    //     {
    //         Serial.println("Failed to process message.");
    //     }
    // }
    // else
    // {
    //     Serial.println("Key 'stream' not found in JSON");
    // }
}

// bool MQTTHandler::processMessage(const char *message)
bool MQTTHandler::processMessage(std::string &message)
{
    static int stat = atoi(_message.c_str());
    static int lastStat = 0;

    log_d("stat = %d", stat);
    if (lastStat == stat)
    {
        return false;
    }

    lastStat = stat;
    message = _message;

    return true;

    // Serial.print("Processing message: ");
    // Serial.println(message);

    // stat = atoi(message);

    // Serial.printf("stat = %d\n", stat);

    // if (lastStat != stat)
    // {
    //     Serial.println("Data change");
    //     MySerial.printf("{"
    //                     "\"stream\" : %d }",
    //                     stat);
    //     lastStat = stat;
    //     Serial.printf("lastStat = %d\n", lastStat);
    //     return false;
    // }
    // else
    // {
    //     Serial.println("do nothing");
    //     return true;
    // }
}

void MQTTHandler::init()
{
    pinMode(BUILTIN_LED, OUTPUT);
    Serial.begin(115200);
    MySerial.begin(115200, SERIAL_8N1, 16, 17);
    _wifi.init();
    client.setServer(mqtt_server, mqtt_port);
    // client.setCallback(callback);
    client.setCallback(MQTTHandler::callback);

    _taskThread = std::thread(&MQTTHandler::taskFunc, this);
}

// void MQTTHandler::loop()
// {
//     if (!client.connected())
//     {
//         reconnect();
//     }
//     client.loop();

//     // unsigned long now = millis();
//     // if (now - lastMsg > 2000) {
//     //     lastMsg = now;
//     //     ++value;
//     // }

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

void MQTTHandler::taskFunc()
{
    while (_running)
    {
        if (!client.connected())
        {
            reconnect();
        }
        client.loop();
    }
}
