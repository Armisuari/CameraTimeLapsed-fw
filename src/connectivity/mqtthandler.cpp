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
            client.subscribe("angkasaConfig");
            client.subscribe("angkasa/deviceSetConfig");
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
    
    // thisconfig = "";
    _message = "";
    
    // Convert payload to string
    for (int i = 0; i < length; i++)
    {
        _message += (char)payload[i];
    }

    const int payloadSize = 1024;
    char _thisconfig[payloadSize];

    for (int i = 0; i < payloadSize - 1; ++i) {
        _thisconfig[i] = (char)payload[i];
    }
    _thisconfig[payloadSize - 1] = '\0';  // Null-terminate the string

    MQTTHandler::handleconfig(_thisconfig, strlen(_thisconfig));
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

void MQTTHandler::handleconfig(const char* thisconfig,unsigned int length){


    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, thisconfig, length);
    if (error) {
        Serial.print("deserializeJson() failed: ");
        Serial.println(error.c_str());
        return;
    }

    if (doc.containsKey("Shutter")) {
        const char* SHUTTER = doc["Shutter"];
        Serial.print("SHUTTER Data: ");
        Serial.println(SHUTTER);
        
        if (strcmp(SHUTTER, "AUTO") == 0) {
            shutterdata = 0;
            Serial.println("Data SHUTTER set to 0");
        }
        else if (strcmp(SHUTTER, "1/125") == 0) {
            shutterdata = 1;
            Serial.println("Data SHUTTER set to 1");
        }
        else if (strcmp(SHUTTER, "1/250") == 0) {
            shutterdata = 2;
            Serial.println("Data SHUTTER set to 2");
        }
        else if (strcmp(SHUTTER, "1/500") == 0) {
            shutterdata = 3;
            Serial.println("Data SHUTTER set to 3");
        }
        else if (strcmp(SHUTTER, "1/1000") == 0) {
            shutterdata = 4;
            Serial.println("Data SHUTTER set to 4");
        }
        else if (strcmp(SHUTTER, "1/2000") == 0) {
            shutterdata = 5;
            Serial.println("Data SHUTTER set to 5");
        }
        else{
            Serial.println("Failed to handle shutter data");
        }
    }

    if (doc.containsKey("ISO")) {
        const char* iso = doc["ISO"];
        Serial.print("iso Data: ");
        Serial.println(iso);
        
        if (strcmp(iso, "800") == 0) {
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
        else{
            Serial.println("Failed to handle iso data");
        }
    }

    if (doc.containsKey("AWB")) {
        const char* awb = doc["AWB"];
        Serial.print("awb Data: ");
        Serial.println(awb);

        if (strcmp(awb, "AUTO") == 0) {
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

        else{
            Serial.println("Failed to handle awb data");
        }

    }

    if (doc.containsKey("EVCOMP")) {
        const char* ev = doc["EVCOMP"];
        Serial.print("ev Data: ");
        Serial.println(ev);

        if (strcmp(ev, "2.0") == 0) {
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

        else{
            Serial.println("Failed to handle ev data");
        }
    }

    _message = "{" + std::string("\"shutter\":") + std::to_string(shutterdata) + "," + 
               "\"iso\":" + std::to_string(isodata) + 
               "\"awb\":" +  std::to_string(awbdata) + 
               "\"ev\":" + std::to_string(evdata) + "}";

    Serial.println(_message.c_str());
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

    bool res = client.publish("angkasa/settings", message.c_str());

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
