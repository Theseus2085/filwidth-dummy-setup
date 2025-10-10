#include <Arduino.h>
#include <Wire.h>
#include <stdlib.h>

constexpr uint8_t kI2CAddress = 0x23;            // 7-bit I2C address for this dummy sensor
constexpr int kSdaPin = 21;                      // default ESP32 devkit SDA pin
constexpr int kSclPin = 22;                      // default ESP32 devkit SCL pin
constexpr unsigned long kIntervalMs = 15000UL;   // 15 seconds between value updates
constexpr float kFloatValues[] = {1.6500f, 1.7500f, 1.8500f};
constexpr size_t kFloatValueCount = sizeof(kFloatValues) / sizeof(kFloatValues[0]);

volatile float gCurrentValue = kFloatValues[0];
unsigned long gLastSwitchMillis = 0;
size_t gCurrentIndex = 0;

void handleI2CRequest();

void setup() {
  Serial.begin(115200);
  Serial.println(F("I2C float broadcaster starting up..."));

  Wire.begin(kI2CAddress, kSdaPin, kSclPin);  // configure ESP32 as an I2C slave on custom pins
  Wire.onRequest(handleI2CRequest);

  gLastSwitchMillis = millis();

  Serial.print(F("Initial value: "));
  Serial.println(kFloatValues[gCurrentIndex], 3);
  Serial.print(F("I2C address 0x"));
  Serial.println(kI2CAddress, HEX);
}

void loop() {
  const unsigned long now = millis();
  if (now - gLastSwitchMillis < kIntervalMs) {
    return;
  }

  gLastSwitchMillis = now;
  gCurrentIndex = (gCurrentIndex + 1) % kFloatValueCount;

  noInterrupts();
  gCurrentValue = kFloatValues[gCurrentIndex];
  interrupts();

  Serial.print(F("Updated value: "));
  Serial.println(gCurrentValue, 3);
}

void handleI2CRequest() {
  float valueCopy;

  noInterrupts();
  valueCopy = gCurrentValue;
  interrupts();

  char buffer[12];
  dtostrf(valueCopy, 1, 3, buffer);  // produce "xx.xxx" style output with 3 decimals

  Wire.write(buffer);
  Wire.write('\n');
}