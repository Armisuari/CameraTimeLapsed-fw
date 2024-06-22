#include <Arduino.h>
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

void BLEsendCommand(byte *command)
{
    //  pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
    pRemoteCharacteristic->writeValue(command, sizeof(command));
}
void connectToWiFi()
{
    WiFi.begin(SSID, PASSWORD);
    Serial.println("Connecting to GoPro AP..");
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("Connected to GoPro AP");
}

void httpGetRequest(const char *url)
{
    // Check WiFi connection status
    if (WiFi.status() == WL_CONNECTED)
    {
        HTTPClient http;

        // Your Domain name with URL path or IP address with path
        http.begin(url);

        // Send HTTP GET request
        int httpResponseCode = http.GET();

        if (httpResponseCode > 0)
        {
            Serial.print("HTTP Response code: ");
            Serial.println(httpResponseCode);
            String payload = http.getString();
            Serial.println(payload);
        }
        else
        {
            Serial.print("Error code: ");
            Serial.println(httpResponseCode);
        }
        // Free resources
        http.end();
    }
    else
    {
        Serial.println("WiFi Disconnected");
    }
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
    }     // onResult
};        // MyAdvertisedDeviceCallbacks

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
} // End of setup.

// This is the Arduino main loop function.
void loop()
{

    if (doConnect == true)
    {
        if (connectToServer())
        {
            Serial.println("We are now connected to the BLE Server.");
            // entah kenapa mesti kirim string nomor dulu biar bisa connect to BLE GoPro
            String newValue = "1";
            byte shutter_command[] = {0x03, 0x01, 0x01, 0x01};
            Serial.println("Confirm Connected");

            pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
        }
        else
        {
            Serial.println("We have failed to connect to the server; there is nothin more we will do.");
        }
        doConnect = false;
    }

    // If we are connected to a peer BLE Server, update the characteristic each time we are reached
    // with the current time since boot.
    if (connected)
    {

        if (!APenabled)
        {
            byte enable_AP_command[] = {0x03, 0x17, 0x01, 0x01};
            pRemoteCharacteristic->writeValue(enable_AP_command, sizeof(enable_AP_command));
            APenabled = true;
            Serial.println("Access Point Enabled");
            connectToWiFi();
        }
        httpGetRequest("http://10.5.5.9/gp/gpControl/setting/88/10");
        delay(3000);
        byte shutter_command[] = {0x03, 0x01, 0x01, 0x01};
        BLEsendCommand(shutter_command);
        delay(5000);
        httpGetRequest("http://10.5.5.9/gp/gpControl/setting/88/50");
        delay(3000);
        httpGetRequest("http://10.5.5.9/gp/gpControl/setting/88/100");
        delay(3000);
        httpGetRequest("http://10.5.5.9/gp/gpControl/setting/87/100");
        delay(3000);
        httpGetRequest("http://10.5.5.9/gp/gpControl/setting/87/85");
        delay(3000);
        httpGetRequest("http://10.5.5.9/gp/gpControl/setting/87/70");
        delay(3000);
        httpGetRequest("http://10.5.5.9/gp/gpControl/setting/87/0");
        delay(3000);
        httpGetRequest("http://10.5.5.9/gp/gpControl/setting/84/0");
        delay(3000);
        httpGetRequest("http://10.5.5.9/gp/gpControl/setting/84/6");
        delay(3000);
        httpGetRequest("http://10.5.5.9/gp/gpControl/setting/84/2");
        delay(3000);
        httpGetRequest("http://10.5.5.9/gp/gpControl/setting/84/3");
        delay(3000);
    }
    else if (doScan)
    {
        BLEDevice::getScan()->start(0); // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
    }

    delay(5000); // Delay a second between loops.
} // End of loop