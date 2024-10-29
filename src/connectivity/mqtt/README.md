MQTT Handler & MQTT Publish Source
====================

# MQTT Handler: Concept

MQTT Handler is represent a single connection to a single MQTT broker, its handle broker connection, and message publishing. Currently topic subscribing and incoming message not yet handled. If the application connect to more than 1 MQTT broker / server, then we can create multiple instance of MQTT handler, each for every broker / server.

To accomplish it's goal, the MQTT Handler have it's own task. The task configuration defined in `MQTTHandlerDefinitions.h`, this configuration have default value but can be overriden by users.

## MQTT Handler: Task Default Value
1. Task Name = MQTTHANDLERTASK
2. Task Priority = 1
3. Task Stack Size = 4KB

To publish MQTT message, the application cannot call directly to MQTT Handler, instead there's provided interface using MQTT Publish Source (explained below).

---
# MQTT Publish Source: Concept

MQTT Publish Source is an object that use as interface to publish message to specific topic, the application need to create an instance for every MQTT topic it's intended to publish to. This interface defined by abstract class named `MQTTPublishSource` and need to be registered on MQTT Handler object via `addPublishSource(MQTTPublishSource* pubSource)` function. The `MQTTPublishSource` have pure virtual function, and the application need to create a subclass that implement the virtual function.

When the application writing to MQTT publish source, the data is enqueued and immediately returned without blocking, regardless of MQTT connection status as long as there's available space in the buffer. Internally it implement queueing mechanism using freeRTOS MessageBuffer, and the message buffer capacity is specified at `MQTTPublishSource` constructor.

![MQTT Publish Source Concept Diagram](../../doc/common-component-esp32-MQTTPublishSource.drawio.png "MQTT Publish Source Concept Diagram")

To optimize the buffer usage, the MQTT publish source also can act as converter / transalaor between raw / original data format to intended MQTT message format, this behavior is optional, but highly recommended since storing the message in raw data form usualy more compact than storing final message format (ie. JSON message). see section usage B below for more information.

## MQTT Publish Source: Caution
1. `MQTTPublishSource` buffer capacity is __specified in Bytes__, not in number of message;
2. The MessageBuffer is not thread safe, and no locking / mutex mechanism implemented inside `MQTTPublishSource` therefore, it is recommended that only single task responsible writing to single instance of `MQTTPublishSource`.

## MQTT Publish Source: Example Usage A, Simple usage

1. For this example application, we Create a subclass to `MQTTPublishSource`, and named it `SensorDataPublishSource` class:
```cpp
class SensorDataPubSource : public MQTTPublishSource
{
public:
    SensorDataPubSource(String topic, size_t bufferSize);
//....................
// other declaration
//....................
};
```
In this case, the `SensorDataPublishSource` doesn't define topic and buffer size, so its need to get topic and bufferSize via constructor and pass it to MQTTPublishSource:
```cpp
// SensorDataPublishSource implementation

SensorDataPubSource::SensorDataPubSource(String topic, size_t bufferSize) : 
MQTTPublishSource(topic, bufferSize)
{
}
```
2. The application need to publish JSON payload, here's the example format:
```json
{
    "data": {
        "DO1": 8.75,   
        "DO2": 97.51,   
        "TDO": 25.66,   
        "ORP": 127.25,   
        "TORP": 26.76   
    },                
    "time": 1672942995,        
    "clientId": "AA BB CC DD EE FF"
}
```
3. The application produce the JSON String and write the string to `SensorDataPublishSource` using `writeRawData()` function:
```cpp
// ................. other application code

Mqtt_Handler mHandler;
SensorDataPublishSource dataPublishSource(topic, bufferSize);

mHandler.addPublishSource(&dataPublishSource); //-----------------> register the publish source

// ................. other application code

String JSONString;
//................... construting json string data
//................... like described in step no 2 above


dataPublishSource.writeRawData(JSONString.c_str(), JSONString.length());
```
The String is now appended to the internal buffer of `MQTTPublishSource`;
4. We need to implement the virtual function `readMQTTPayload()`, since we already write formatted payload as raw data, we only need to cast the raw data into String:
```cpp
// SensorDataPublishSource implementation

String SensorDataPubSource::readMQTTPayload()
{
    char buffer[512];
    size_t result = readRawData(buffer, 512)
    if(result <= 0)
    {
        return String();
    }

    return String(buffer, result);
}

size_t SensorDataPubSource::readMQTTPayload(uint8_t* buff, size_t length)
{
    return readRawData(buff, length);
}
```

### MQTT Publish Source: Example Usage B, Optimized buffer memory

1. like example A, we create `SensorDataPublishSource` class;
2. The MQTT message format is same as example A above;
3. In this example we do not store the JSON message directly, instead we define a data structure to hold all the data except for client ID. The client ID wouldn't change and can be stored as private variable. So we create struct `SensorData_t` and supplied the client ID on constructor:
```cpp
typedef struct{
    float DOmgL;
    float DOpercent;
    float ORPmV;
    float DOtempCelcius;
    float ORPtempCelcius;
    uint32_t timestamp;
} SensorData_t;

class SensorDataPubSource : public MQTTPublishSource
{
public:
    SensorDataPubSource(const char* clientID, String topic, size_t bufferSize);
//....................
// other declaration
//....................
private:
    char _clientID[18]; //----------> Client ID stored as private variable
//....................
// other declaration
//....................
};
```
4. we create new function called `writeSensorData(SensorData_t data)` to store the `SensorData_t` into buffer:
```cpp
// SensorDataPublishSource implementation

bool SensorDataPubSource::writeSensorData(SensorData_t data)
{
    return writeRawData(&data, sizeof(SensorData_t)) == sizeof(SensorData_t);   
}
```
5. We need to implement function `readMQTTPayload()` that return JSON message formatted like described in step no. 2, so we need to convert `struct SensorData_t` to JSON message. Here is the difference with example usage A.
```cpp
String SensorDataPubSource::_formatSensorDataJson(SensorData_t data)
{
    // another function to convert SensorData_t to JSON String
    //............
}

String SensorDataPubSource::readMQTTPayload()
{
    SensorData_t data;
    String payload;
    if(readRawData(&data, sizeof(SensorData_t)) > 0)
    {
        payload = _formatSensorDataJson(data);
    }

    return payload;
}

size_t SensorDataPubSource::readMQTTPayload(uint8_t* buff, size_t length)
{
    SensorData_t data;
    size_t result = 0;
    String payload;
    if(readRawData(&data, sizeof(SensorData_t)) == sizeof(SensorData_t))
    {
        payload = _formatSensorDataJson(data);
        if(payload.length() < length)
        {
            payload.getBytes(buff, length);
            result = payload.length();
        }
    }

    return result;
}
```

In example B, each message use fixed amount of buffer memory which is `sizeof(SensorData_t)` = 24 Bytes. Meanwhile the message in example A stored directly as JSON string even if minified, is 120 bytes. The length of JSON message will vary depending on the sensor values, but won't make a difference. If we allocate 4KB or 4096 bytes to the buffer, we can calculate the maximum number of messages stored as:

- example A: 4096 / (120 + 4 * ) = 33 messages
- example B: 4096 / (24 + 4 * ) = 146 messages

*messageBuffer have overhead for every message stored, in this case the overhead is 4 bytes.