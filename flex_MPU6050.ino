#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define BUZZER_PIN 10  
const int flexPins[] = {1, 2, 3, 4, 5}; 
Adafruit_MPU6050 mpu;

BLECharacteristic* pCharacteristic = NULL;
bool deviceConnected = false;
#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

class MyCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* p) { deviceConnected = true; }
    void onDisconnect(BLEServer* p) { 
      deviceConnected = false; 
      p->getAdvertising()->start(); 
    }
};

void setup() {
  Serial.begin(115200);
  pinMode(BUZZER_PIN, OUTPUT);
  
  Wire.begin(8, 9); 

  Wire.beginTransmission(0x68);
  Wire.write(0x6B); 
  Wire.write(0);    
  Wire.endTransmission();

  if (!mpu.begin()) {
    Serial.println("--- ERROR: MPU6050 not found! Check wiring ---");
  } else {
    Serial.println("--- SUCCESS: MPU6050 Online ---");

    mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
    mpu.setGyroRange(MPU6050_RANGE_500_DEG);
    mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  }

  BLEDevice::init("Smart_Glove_S3");
  BLEServer* pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ | 
                      BLECharacteristic::PROPERTY_NOTIFY |
                      BLECharacteristic::PROPERTY_WRITE
                    );
  pCharacteristic->addDescriptor(new BLE2902());
  pService->start();
  pServer->getAdvertising()->start();

  Serial.println("--- System Online: Ready to transmit ---");
}

void loop() {
  int currentValues[5];
  sensors_event_t a, g, temp;

  mpu.getEvent(&a, &g, &temp); 

  Serial.print("Flex: ");
  for (int i = 0; i < 5; i++) {
    currentValues[i] = analogRead(flexPins[i]);
    Serial.print(currentValues[i]);
    if (i < 4) Serial.print(",");
  }

  Serial.print(" | Accel: ");
  Serial.print(a.acceleration.x); Serial.print(",");
  Serial.print(a.acceleration.y); Serial.print(",");
  Serial.print(a.acceleration.z);
  Serial.println();

  if (currentValues[4] >= 1000) {
    digitalWrite(BUZZER_PIN, HIGH);
    Serial.println(">> ALERT: Pin 5 Active!");
  } else {
    digitalWrite(BUZZER_PIN, LOW);
  }

if (deviceConnected) {
    char packet[200]; 

    sprintf(packet, 
            "Fingers: %d, %d, %d, %d, %d\nAccel: X:%.1f Y:%.1f Z:%.1f", 
            currentValues[0], currentValues[1], currentValues[2], currentValues[3], currentValues[4],
            a.acceleration.x, a.acceleration.y, a.acceleration.z);
            
    pCharacteristic->setValue(packet);
    pCharacteristic->notify();
  }

  delay(500); 
}
