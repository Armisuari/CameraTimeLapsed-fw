#include <Arduino.h>

#include <stdio.h>
#include <pb_encode.h>
#include <pb_decode.h>

#include "live_stream.pb.h"

#include "NimBLEDevice.h"
#include <WiFi.h>
#include <HTTPClient.h>

// The remote service we wish to connect to.
static BLEUUID serviceUUID("0000fea6-0000-1000-8000-00805f9b34fb");
// static BLEUUID serviceUUID("0xfea6");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("b5f90072-aa8d-11e3-9046-0002a5d5c51b");
// static BLEUUID charUUID("b5f90001-aa8d-11e3-9046-0002a5d5c51b");

const char *SSID = "GoPro Hero 9 Angkasa";
const char *PASSWORD = "NPc-hg6-S5S";

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static boolean APenabled = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;

/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
    /**
     * Called for each advertising BLE server.
     */

    /*** Only a reference to the advertised device is passed now
      void onResult(BLEAdvertisedDevice advertisedDevice) { **/
    void onResult(BLEAdvertisedDevice *advertisedDevice)
    {
        Serial.print("BLE Advertised Device found: ");
        Serial.println(advertisedDevice->toString().c_str());

        // We have found a device, let us now see if it contains the service we are looking for.
        /********************************************************************************
            if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
        ********************************************************************************/
        if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(serviceUUID))
        {

            BLEDevice::getScan()->stop();
            /*******************************************************************
                  myDevice = new BLEAdvertisedDevice(advertisedDevice);
            *******************************************************************/
            myDevice = advertisedDevice; /** Just save the reference now, no need to copy the object */
            doConnect = true;
            doScan = true;

        } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class MyClientCallback : public BLEClientCallbacks
{
    void onConnect(BLEClient *pclient)
    {
        Serial.println("Connectedd");
    }

    void onDisconnect(BLEClient *pclient)
    {
        connected = false;
        APenabled = false;
        Serial.println("onDisconnect");
    }
    /***************** New - Security handled here ********************
    ****** Note: these are the same return values as defaults ********/
    uint32_t onPassKeyRequest()
    {
        Serial.println("Client PassKeyRequest");
        return 123456;
    }
    bool onConfirmPIN(uint32_t pass_key)
    {
        Serial.print("The passkey YES/NO number: ");
        Serial.println(pass_key);
        return true;
    }

    void onAuthenticationComplete(ble_gap_conn_desc desc)
    {
        Serial.println("Starting BLE work!");
    }
    /*******************************************************************/
};

bool connectToServer();
void onResponse(NimBLERemoteCharacteristic *pCharacteristic, uint8_t *pData, size_t length, bool isNotify);
void setLiveStreamMode(NimBLERemoteCharacteristic *pRequestCharacteristic, std::string url);
static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify);

void setup()
{
    Serial.begin(115200);
    Serial.println("Starting ESP32 BLE Client application...");
    BLEDevice::init("ESP32 MCU");
    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    BLEScan *pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);

    if (doConnect == true)
    {
        if (connectToServer())
        {
            Serial.println("We are now connected to the BLE Server.");
            // entah kenapa mesti kirim string nomor dulu biar bisa connect to BLE GoPro
            // String newValue = "1";
            // Serial.println("Confirm Connected");
            // byte shutter_command[] = {0x03, 0x01, 0x01, 0x01};
            // pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
            std::string rtmpURL = "rtmp://angkasatimelapse.com/live/mykey";
            setLiveStreamMode(pRemoteCharacteristic, rtmpURL);
        }
        else
        {
            Serial.println("We have failed to connect to the server; there is nothin more we will do.");
        }
        doConnect = false;
    }
}

void loop()
{
}

bool connectToServer()
{
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());

    BLEClient *pClient = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice); // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr)
    {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(serviceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr)
    {
        Serial.print("Failed to find our characteristic UUID: ");
        Serial.println(charUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if (pRemoteCharacteristic->canRead())
    {
        std::string value = pRemoteCharacteristic->readValue();
        Serial.print("The characteristic value was: ");
        Serial.println(value.c_str());
    }

    if (pRemoteCharacteristic->canNotify())
        pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
    return true;
}

// void onResponse(NimBLERemoteCharacteristic* pCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
//     Serial.print("Response received: ");
//     for (size_t i = 0; i < length; ++i) {
//         Serial.printf("%02X ", pData[i]);
//     }
//     Serial.println();

//     // Parse the protobuf message
//     proto_ResponseGeneric response;
//     pb_istream_t stream = pb_istream_from_buffer(pData + 2, length - 2);
//     if (!pb_decode(&stream, proto_ResponseGeneric_fields, &response)) {
//         Serial.println("Failed to parse response");
//         return;
//     }

//     Serial.println("Successfully parsed response");
//     // Print or handle the response data as needed
// }

void setLiveStreamMode(NimBLERemoteCharacteristic *pRequestCharacteristic, std::string url)
{
    // proto_RequestSetTurboActive request = proto_RequestSetTurboActive_init_default;
    open_gopro_RequestSetLiveStreamMode request = open_gopro_RequestSetLiveStreamMode_init_default;
    // request.active = enable;
    strcpy(request.url, url.c_str());

    uint8_t buffer[128];
    pb_ostream_t stream = pb_ostream_from_buffer(buffer + 3, sizeof(buffer) - 3);
    if (!pb_encode(&stream, open_gopro_RequestSetLiveStreamMode_fields, &request))
    {
        Serial.println("Failed to encode request");
        return;
    }

    size_t messageLength = stream.bytes_written;
    buffer[0] = 3 + messageLength; // Total length
    buffer[1] = 0xF1;              // Feature ID
    buffer[2] = 0x79;              // Action ID

    pRequestCharacteristic->writeValue(buffer, 3 + messageLength, true);
    Serial.println("Turbo mode set request sent");
}

static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic,
    uint8_t *pData,
    size_t length,
    bool isNotify)
{
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char *)pData);
}
