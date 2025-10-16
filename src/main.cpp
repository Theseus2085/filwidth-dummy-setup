#include <Arduino.h>
#include <Wire.h>

#if defined(ARDUINO_ARCH_ESP32)
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#endif

constexpr uint8_t kI2CAddress = 0x42;            // 7-bit I2C address for this dummy sensor
constexpr float kOutputValue = 1.75f + (0.15f * 1.75f);  // single value served over I2C
constexpr unsigned long kLedBlinkDuration = 100; // LED on duration in milliseconds
constexpr unsigned long kSlowBlinkInterval = 2000; // Slow blink half-period in milliseconds

#if defined(ARDUINO_ARCH_ESP32)
constexpr int kSdaPin = 21;                      // Default SDA pin on ESP32 DevKit
constexpr int kSclPin = 22;                      // Default SCL pin on ESP32 DevKit
#endif

#if defined(LED_BUILTIN)
constexpr uint8_t kLedPin = LED_BUILTIN;
#elif defined(ARDUINO_ARCH_ESP32)
constexpr uint8_t kLedPin = 2;                   // Fallback for ESP32 boards without LED_BUILTIN
#else
constexpr uint8_t kLedPin = 13;                  // Common fallback for AVR boards
#endif

char gDigitBuffer[6] = {};
volatile unsigned long gLedOffTime = 0;          // millis() tick when LED should turn off
unsigned long gSlowBlinkNextToggle = 0;         // next scheduled toggle for slow blink
bool gSlowBlinkState = false;                   // false = LED off, true = LED on

#if defined(ARDUINO_ARCH_ESP32)
portMUX_TYPE gLedMux = portMUX_INITIALIZER_UNLOCKED;

inline unsigned long readLedOffTime() {
  portENTER_CRITICAL(&gLedMux);
  unsigned long copy = gLedOffTime;
  portEXIT_CRITICAL(&gLedMux);
  return copy;
}

inline void writeLedOffTime(unsigned long value) {
  portENTER_CRITICAL(&gLedMux);
  gLedOffTime = value;
  portEXIT_CRITICAL(&gLedMux);
}
#else
inline unsigned long readLedOffTime() {
  noInterrupts();
  unsigned long copy = gLedOffTime;
  interrupts();
  return copy;
}

inline void writeLedOffTime(unsigned long value) {
  noInterrupts();
  gLedOffTime = value;
  interrupts();
}
#endif

void handleI2CRequest();

void setup() {
  Serial.begin(115200);
  Serial.println(F("I2C float broadcaster starting up..."));

  pinMode(kLedPin, OUTPUT);
  digitalWrite(kLedPin, LOW);                    // LED off initially

#if defined(ARDUINO_ARCH_ESP32)
  Wire.begin(kSdaPin, kSclPin, kI2CAddress);     // configure ESP32 as I2C slave on selected pins
#else
  Wire.begin(kI2CAddress);                       // configure board as I2C slave (defaults)
#endif
  Wire.onRequest(handleI2CRequest);

  int scaled = static_cast<int>(kOutputValue * 10000.0f + 0.5f);
  for (int idx = 4; idx >= 0; --idx) {
    gDigitBuffer[idx] = static_cast<char>('0' + (scaled % 10));
    scaled /= 10;
  }
  gDigitBuffer[5] = '\0';

  gSlowBlinkNextToggle = millis();

  Serial.print(F("Constant value: "));
  Serial.println(kOutputValue, 3);
  Serial.print(F("I2C address 0x"));
  Serial.println(kI2CAddress, HEX);
  Serial.print(F("Digits on I2C: "));
  Serial.println(gDigitBuffer);
}

void loop() {
  const unsigned long now = millis();

  // Handle fast blink request pulse
  unsigned long ledOffCopy = readLedOffTime();

  if (ledOffCopy != 0) {
    if (now >= ledOffCopy) {
      digitalWrite(kLedPin, LOW);
      writeLedOffTime(0);
      gSlowBlinkState = false;
      gSlowBlinkNextToggle = now + kSlowBlinkInterval;
    }
  } else {
    // Run slow heartbeat blink when no I2C activity is pending
    if (now >= gSlowBlinkNextToggle) {
      gSlowBlinkState = !gSlowBlinkState;
      digitalWrite(kLedPin, gSlowBlinkState ? HIGH : LOW);
      gSlowBlinkNextToggle = now + kSlowBlinkInterval;
    }
  }
  delay(10);  // Yield to background tasks while waiting for I2C requests
}

void handleI2CRequest() {
  digitalWrite(kLedPin, HIGH);                   // Turn LED on
  unsigned long offTime = millis() + kLedBlinkDuration;
  writeLedOffTime(offTime);
  gSlowBlinkState = true;                       // ensure LED reads as on during pulse
  gSlowBlinkNextToggle = millis() + kSlowBlinkInterval;
  Serial.println(F("I2C request received"));
  Wire.write(reinterpret_cast<uint8_t*>(gDigitBuffer), 5);
}