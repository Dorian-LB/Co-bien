#include <Wire.h>
 #include <SPI.h>
  #include <WiFi.h>
  #include "FS.h"
#define SLAVE_ADDRESS 0x28  // I2C address of the PIC16LF1559

// Register addresses
#define REG_RESET_STATE        0x00  // Touch reset parameter
#define REG_TOUCH_STATE        0x01  // Touch state parameter (bit field)
#define REG_TOUCH_DEVIATION    0x10  // Touch deviation (2 bytes)
#define REG_TOUCH_THRESHOLD    0x30  // Touch threshold (2 bytes)
#define REG_TOUCH_SCALING      0x50  // Touch deviation scaling (2 bytes)

// Function to read a single byte from a register
uint8_t readRegister8(uint8_t reg) {
  Wire.beginTransmission(SLAVE_ADDRESS);
  Wire.write(reg);                  // Write the register address
  Wire.endTransmission(false);      // Send stop condition after address
  Wire.requestFrom(SLAVE_ADDRESS, 1); // Request 1 byte
  return Wire.available() ? Wire.read() : 0;
}

// Function to read two bytes from a register
uint16_t readRegister16(uint8_t reg) {
  Wire.beginTransmission(SLAVE_ADDRESS);
  Wire.write(reg);                  // Write the register address
  Wire.endTransmission(false);      // Send stop condition after address
  Wire.requestFrom(SLAVE_ADDRESS, 2); // Request 2 bytes
  uint8_t highByte = Wire.available() ? Wire.read() : 0;
  uint8_t lowByte = Wire.available() ? Wire.read() : 0;
  return (highByte << 8) | lowByte; // Combine high and low bytes
}

void setup() {
  Serial.begin(115200);       // Initialize Serial Monitor
  Wire.begin();               // Initialize I2C as Master
  Wire.setClock(100000);      // Set I2C clock speed (100kHz)
}

void loop() {
  Serial.println("Received Touch Data:");

  // Read the touch reset parameter (1 byte)
  uint8_t resetState = readRegister8(REG_RESET_STATE);
  Serial.print("1. Touch Reset Parameter: ");
  Serial.println(resetState, HEX);

  // Read the touch state parameter (1 byte, bit field)
  uint8_t touchState = readRegister8(REG_TOUCH_STATE);
  Serial.print("2. Touch State Parameter (Bit Field): 0x");
  Serial.print(touchState, HEX);
  Serial.print(" (");
  for (int b = 7; b >= 0; b--) {
    Serial.print((touchState >> b) & 1); // Print bit field
    if (b > 0) Serial.print(",");
  }
  Serial.println(")");

  // Read the touch deviation (2 bytes)
  uint16_t touchDeviation = readRegister16(REG_TOUCH_DEVIATION);
  Serial.print("3. Touch Deviation (Touch Sensor): ");
  Serial.println((uint8_t)(touchDeviation >> 8)); // High byte
  Serial.print("4. Touch Deviation (Proximity Sensor): ");
  Serial.println((uint8_t)(touchDeviation & 0xFF)); // Low byte

  // Read the touch threshold (2 bytes)
  uint16_t touchThreshold = readRegister16(REG_TOUCH_THRESHOLD);
  Serial.print("5. Touch Threshold (Touch Sensor): ");
  Serial.println((uint8_t)(touchThreshold >> 8)); // High byte
  Serial.print("6. Touch Threshold (Proximity Sensor): ");
  Serial.println((uint8_t)(touchThreshold & 0xFF)); // Low byte

  // Read the touch deviation scaling (2 bytes)
  uint16_t touchScaling = readRegister16(REG_TOUCH_SCALING);
  Serial.print("7. Touch Deviation Scaling (Touch Sensor): ");
  Serial.println((uint8_t)(touchScaling >> 8)); // High byte
  Serial.print("8. Touch Deviation Scaling (Proximity Sensor): ");
  Serial.println((uint8_t)(touchScaling & 0xFF)); // Low byte

  Serial.println("-------------------------------");
  delay(1000); // Delay between readings
}

