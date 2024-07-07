#pragma once
#include <string>
#include <memory>
#include <atomic>
#include <thread>
#include <chrono>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "WifiHandler.h"
#include "CONFIG.h"
#include <HardwareSerial.h>

class MQTTHandler
{
public:
    MQTTHandler(const char *ssid, const char *password, const char *mqtt_server, int mqtt_port);
    void init();
    bool processMessage(std::string &message);
    bool publish(std::string message);

private:
    void reconnect();
    static void callback(char *topic, byte *payload, unsigned int length);
    void handleCallback(char *topic, byte *payload, unsigned int length);

    const char *ssid;
    const char *password;
    const char *mqtt_server;
    int mqtt_port;

    WiFiClient espClient;
    PubSubClient client;
    unsigned long lastMsg;
    int value;
    static constexpr int MSG_BUFFER_SIZE = 50;
    char msg[MSG_BUFFER_SIZE];
    int lastStat; // Declare lastStat as a member variable
    int stat;

    static MQTTHandler *instance;

    std::string _message;
    String clientId = "ESP32Client-";

    static void taskFunc(void *pvParam);
};
