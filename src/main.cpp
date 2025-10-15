#include <Arduino.h>
#include <Wire.h>

constexpr uint8_t kI2CAddress = 0x20;            // 7-bit I2C address for this dummy sensor
constexpr int kSdaPin = 21;                      // default ESP32 devkit SDA pin
constexpr int kSclPin = 22;                      // default ESP32 devkit SCL pin
constexpr float kOutputValue = 1.75+(0.15*1.75);            // single value served over I2C

char gDigitBuffer[6] = {};

void handleI2CRequest();

void setup() {
  Serial.begin(115200);
  Serial.println(F("I2C float broadcaster starting up..."));

  Wire.begin(kI2CAddress, kSdaPin, kSclPin);  // configure ESP32 as an I2C slave on custom pins
  Wire.onRequest(handleI2CRequest);

  int scaled = static_cast<int>(kOutputValue * 10000.0f + 0.5f);
  for (int idx = 4; idx >= 0; --idx) {
    gDigitBuffer[idx] = static_cast<char>('0' + (scaled % 10));
    scaled /= 10;
  }
  gDigitBuffer[5] = '\0';

  Serial.print(F("Constant value: "));
  Serial.println(kOutputValue, 3);
  Serial.print(F("I2C address 0x"));
  Serial.println(kI2CAddress, HEX);
  Serial.print(F("Digits on I2C: "));
  Serial.println(gDigitBuffer);
}

void loop() {
  delay(10);  // Yield to background tasks while waiting for I2C requests
}

void handleI2CRequest() {
  Wire.write(reinterpret_cast<uint8_t*>(gDigitBuffer), 5);
}