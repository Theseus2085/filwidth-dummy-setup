#include <Arduino.h>
#include <Wire.h>
#include <stdlib.h>

constexpr uint8_t kI2CAddress = 0x20;            // 7-bit I2C address for this dummy sensor
constexpr int kSdaPin = 21;                      // default ESP32 devkit SDA pin
constexpr int kSclPin = 22;                      // default ESP32 devkit SCL pin
constexpr float kOutputValue = 1.75+(0.15*1.75);            // single value served over I2C

void handleI2CRequest();

void setup() {
  Serial.begin(115200);
  Serial.println(F("I2C float broadcaster starting up..."));

  Wire.begin(kI2CAddress, kSdaPin, kSclPin);  // configure ESP32 as an I2C slave on custom pins
  Wire.onRequest(handleI2CRequest);

  Serial.print(F("Constant value: "));
  Serial.println(kOutputValue, 3);
  Serial.print(F("I2C address 0x"));
  Serial.println(kI2CAddress, HEX);
}

void loop() {
  delay(10);  // Yield to background tasks while waiting for I2C requests
}

void handleI2CRequest() {
  char buffer[12];
  dtostrf(kOutputValue, 1, 3, buffer);  // produce "xx.xxx" style output with 3 decimals

  Wire.write(buffer);
  Wire.write('\n');
}