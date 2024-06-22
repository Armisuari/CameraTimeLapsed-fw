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
  }   // onResult
};    // MyAdvertisedDeviceCallbacks

/* This is the buffer where we will store our message. */
uint8_t buffer[128];
size_t message_length;
bool status;

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
}

void loop()
{

  // ENCODE
  open_gopro_RequestSetLiveStreamMode message = open_gopro_RequestSetLiveStreamMode_init_zero;

  pb_ostream_t ostream = pb_ostream_from_buffer(buffer, sizeof(buffer));

  message.minimum_bitrate = 720;
  strcpy(message.url, "rtmp://angkasatimelapse.com/live/mykey");
  message.maximum_bitrate = 1080;

  status = pb_encode(&ostream, open_gopro_RequestSetLiveStreamMode_fields, &message);
  message_length = ostream.bytes_written;

  if (!status)
  {
    Serial.printf("Encoding failed: %s\n", PB_GET_ERROR(&ostream));
  }
  byte start_livestream_command[] = {0xF1, 0x79};
  BLEsendCommand(start_livestream_command);

  for (int i = 0; i < 128; i++)
  {
    Serial.print(buffer[i]);
  }

  // DECODE
  open_gopro_RequestSetLiveStreamMode message2 = open_gopro_RequestSetLiveStreamMode_init_zero;

  pb_istream_t istream = pb_istream_from_buffer(buffer, message_length);

  status = pb_decode(&istream, open_gopro_RequestSetLiveStreamMode_fields, &message2);

  if (!status)
  {
    Serial.printf("Decoding failed: %s\n", PB_GET_ERROR(&istream));
  }

  Serial.println(message.url);
  Serial.println(message.minimum_bitrate);
  Serial.println(message.maximum_bitrate);
  Serial.println(sizeof(buffer));
  Serial.println(ostream.bytes_written);
  Serial.println(message2.url);
  Serial.println(message2.minimum_bitrate);
  Serial.println(message2.maximum_bitrate);
  Serial.println(sizeof(buffer));
  delay(3000);
}