#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

// Список сервисов и характеристик
static BLEUUID serviceUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e");
static BLEUUID writeCharUUID("6e400002-b5a3-f393-e0a9-e50e24dcca9e");
static BLEUUID readCharUUID("6e400003-b5a3-f393-e0a9-e50e24dcca9e");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

static BLEAdvertisedDevice *myDevice;
static BLERemoteCharacteristic *readRemoteCharacteristic;
static BLERemoteCharacteristic *writeRemoteCharacteristic;

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, byte *pData, size_t length, bool isNotify)
{
  Serial.print("Notify received: ");

  if (pData[3] == (byte)0x45)
  {
    Serial.println("live");
  }

  else if (pData[3] == (byte)0x14)
  {
    Serial.println("stats");
  }

  // Печатаем весь массив
  for (int i = 0; i < length; i++)
  {
    Serial.print(pData[i], HEX);
  }
  Serial.println();
  digitalWrite(2, LOW);
}

class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
    // Serial.println("onConnect");
  }

  void onDisconnect(BLEClient *pclient)
  {
    connected = false;
    Serial.println("- onDisconnect");
  }
};

bool connectToServer()
{
  Serial.print("Connecting to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  Serial.println(" - Creating BLE Client");
  BLEClient *pClient = BLEDevice::createClient();

  pClient->setClientCallbacks(new MyClientCallback());

  pClient->connect(myDevice);
  Serial.println(" - Connected to server");
  // pClient->setMTU(517);  //set client to request maximum MTU from server (default is 23 otherwise)

  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find primary service UUID: ");
    Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found primary service");

  readRemoteCharacteristic = pRemoteService->getCharacteristic(readCharUUID);
  if (readRemoteCharacteristic == nullptr)
  {
    Serial.print("Failed to find read characteristic UUID: ");
    Serial.println(readCharUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found read characteristic");

  if (readRemoteCharacteristic->canNotify())
  {
    readRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.println(" - Notify for read characteristic registered");
  }

  writeRemoteCharacteristic = pRemoteService->getCharacteristic(writeCharUUID);
  if (writeRemoteCharacteristic == nullptr)
  {
    Serial.print("Failed to find write characteristic UUID: ");
    Serial.println(readCharUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found write characteristic");

  if (writeRemoteCharacteristic->canWrite())
  {
    Serial.println(" - Can write to characteristic");
  }

  connected = true;
  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  void onResult(BLEAdvertisedDevice advertisedDevice)
  {
    if (advertisedDevice.getName() != "") {
      Serial.print("- BLE device found: ");
      Serial.println(advertisedDevice.toString().c_str());
    }

    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID))
    {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;
    }
  }
};

void setup()
{
  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting Arduino BLE Client application...");

  BLEDevice::init("");

  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(15, false);

  pinMode(2, OUTPUT);
}

void loop()
{
  if (doConnect == true)
  {
    if (connectToServer())
    {
      Serial.println("We are now connected to the BLE Server.");
    }
    else
    {
      Serial.println("We have failed to connect to the server; there is nothing more we will do.");
    }
    doConnect = false;
  }

  if (connected)
  {
    digitalWrite(2, HIGH);

    byte live[6] = {0xAA, 0xAA, 0x14, 0x01, 0x04, 0x11};
    byte stats[6] = {0xAA, 0xAA, 0x14, 0x01, 0x11, 0x04};
    byte drlOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2D, 0x01, 0x5B};
    byte drlOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2D, 0x00, 0x5A};
    byte lightsOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x50, 0x01, 0x26};
    byte lightsOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x50, 0x00, 0x27};
    byte fanOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x43, 0x01, 0x35};
    byte fanOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x43, 0x00, 0x34};
    byte fanQuietOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x38, 0x01, 0x4E};
    byte fanQuietOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x38, 0x00, 0x4F};
    byte liftOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2E, 0x01, 0x58};
    byte liftOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2E, 0x00, 0x59};
    byte lock[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x31, 0x01, 0x47};
    byte unlock[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x31, 0x00, 0x46};
    byte transportOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x32, 0x01, 0x44};
    byte transportOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x32, 0x00, 0x45};
    byte rideComfort[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x23, 0x00, 0x54};
    byte rideSport[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x23, 0x01, 0x55};
    byte performanceOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x24, 0x01, 0x52};
    byte performanceOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x24, 0x00, 0x53};
    byte remainderReal[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x3D, 0x01, 0x4B};
    byte remainderEst[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x3D, 0x00, 0x4A};
    byte lowBatLimitOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x37, 0x01, 0x41};
    byte lowBatLimitOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x37, 0x00, 0x40};
    byte usbOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x3C, 0x01, 0x4A};
    byte usbOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x3C, 0x00, 0x4B};
    byte loadDetectOn[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x36, 0x01, 0x40};
    byte loadDetectOff[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x36, 0x00, 0x41};
    byte mute[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2C, 0x00, 0x5B};
    byte unmute[8] = {0xAA, 0xAA, 0x14, 0x03, 0x60, 0x2C, 0x01, 0x5A};
    byte calibration[10] = {0xAA, 0xAA, 0x14, 0x05, 0x60, 0x42, 0x01, 0x00, 0x01, 0x33};

    // writeRemoteCharacteristic->writeValue(stats, sizeof(stats));
    // writeRemoteCharacteristic->writeValue(live, sizeof(live));

    writeRemoteCharacteristic->writeValue(drlOn, sizeof(drlOn));

    delay(4000);

    writeRemoteCharacteristic->writeValue(drlOff, sizeof(drlOff));
  }
  else if (doScan)
  {
    BLEDevice::getScan()->start(10);
  }

  delay(4000);
}
