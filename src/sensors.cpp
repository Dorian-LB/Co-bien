#include "sensors.h"

SensorsManager::SensorsManager(PubSubClient &client) : mqttClient(client) {}

//PUBLIC FUNCTIONS

void SensorsManager::initializeI2C(){
    Wire.begin(21, 22); // SDA, SCL for Wire (I2C0)
    Wire1.begin(25, 26); // SDA, SCL for Wire1 (I2C1)
    Wire.setClock(100000);
    Wire1.setClock(100000);
}

// Function to read the sensor configuration from the JSON file and calls the updateSensorConfiguration function
bool SensorsManager::setupSensors() {
  DynamicJsonDocument doc(2048);
  if(SPIFFS.exists(configFile)) {
    File file = SPIFFS.open(configFile, "r");
    if (file) {
        deserializeJson(doc,file);
        file.close();
      }
  }
  JsonArray sensorsArray = doc["sensors"].as<JsonArray>();
  for (JsonObject obj : sensorsArray) {
      uint8_t touchThreshold = obj["touchThreshold"];
      uint8_t proximityThreshold = obj["proximityThreshold"];
      uint8_t touchScaling = obj["touchScaling"];
      uint8_t proximityScaling = obj["proximityScaling"];

      // Combine the threshold and scaling values
      uint16_t threshold = (touchThreshold << 8) | proximityThreshold;
      uint16_t scaling = (touchScaling << 8) | proximityScaling;

      updateSensorConfiguration(obj["PIC_id"], scaling, threshold);
    }
  return true;
  }

void SensorsManager::handleSensorConfigurationMessage(DynamicJsonDocument& message) {
  int PIC_id = message["PIC"];
  uint8_t touchThreshold = message["touchThreshold"];
  uint8_t proximityThreshold = message["proximityThreshold"];
  uint8_t touchScaling = message["touchScaling"];
  uint8_t proximityScaling = message["proximityScaling"];

  // Combine the threshold and scaling values
  uint16_t threshold = (touchThreshold << 8) | proximityThreshold;
  uint16_t scaling = (touchScaling << 8) | proximityScaling;

  // Update the configuration in the JSON file
  DynamicJsonDocument doc(2048);
  if (SPIFFS.exists(configFile)) {
    File file = SPIFFS.open(configFile, "r");
    if (file) {
      deserializeJson(doc, file);
      file.close();
    }
  }

  JsonArray sensorsArray = doc["sensors"].as<JsonArray>();
  for (JsonObject obj : sensorsArray) {
    if (obj["PIC_id"] == PIC_id) {
      obj["touchThreshold"] = touchThreshold; // Save touch threshold
      obj["touchScaling"] = touchScaling;     // Save touch scaling
      obj["proximityThreshold"] = proximityThreshold; // Save proximity threshold
      obj["proximityScaling"] = proximityScaling;     // Save proximity scaling
      break;
    }
  }

  // Save updated configuration in the json conf file
  saveConfiguration(doc);

  // Update the sensor configuration
  updateSensorConfiguration(PIC_id, scaling, threshold);
}

// Function to send current sensors deviations via MQTT
void SensorsManager::sendSensorDeviation() {

  StaticJsonDocument<512> jsonDoc; // Adjust size based on number of sensors

  // Read deviation parameters for the first PIC (touch and proximity sensors)
  uint16_t deviationPIC1 = readRegister16(Wire, SLAVE1_ADDRESS, REG_TOUCH_DEVIATION);
  uint8_t touchDeviationPIC1 = (deviationPIC1 >> 8) & 0xFF;  // High byte
  uint8_t proximityDeviationPIC1 = deviationPIC1 & 0xFF;     // Low byte
  
  uint8_t PIC1States = readRegister8(Wire, SLAVE1_ADDRESS, REG_TOUCH_STATE); 
  // Extract the touchState (LSB)
  uint8_t touchStatePIC1 = PIC1States & 0x01; // Mask the LSB (bit 0)
  // Extract the proximityState (second bit)
  uint8_t proximityStatePIC1 = (PIC1States >> 1) & 0x01; // Shift right by 1 and mask bit 0

  // Read deviation parameters for the second PIC (touch and proximity sensors)
  uint16_t deviationPIC2 = readRegister16(Wire1, SLAVE2_ADDRESS, REG_TOUCH_DEVIATION);
  uint8_t touchDeviationPIC2 = (deviationPIC2 >> 8) & 0xFF;  // High byte
  uint8_t proximityDeviationPIC2 = deviationPIC2 & 0xFF;     // Low byte

  uint8_t PIC2States = readRegister8(Wire1, SLAVE2_ADDRESS, REG_TOUCH_STATE); 
  // Extract the touchState (LSB)
  uint8_t touchStatePIC2 = PIC2States & 0x01; // Mask the LSB (bit 0)
  // Extract the proximityState (second bit)
  uint8_t proximityStatePIC2 = (PIC2States >> 1) & 0x01; // Shift right by 1 and mask bit 0


  // Add data to JSON
  jsonDoc["PIC1"]["touchDeviation"] = touchDeviationPIC1;
  jsonDoc["PIC1"]["proximityDeviation"] = proximityDeviationPIC1;
  jsonDoc["PIC1"]["touchState"] = touchStatePIC1;
  jsonDoc["PIC1"]["proximityState"] = proximityStatePIC1;
  jsonDoc["PIC2"]["touchDeviation"] = touchDeviationPIC2;
  jsonDoc["PIC2"]["proximityDeviation"] = proximityDeviationPIC2;
  jsonDoc["PIC2"]["touchState"] = touchStatePIC2;
  jsonDoc["PIC2"]["proximityState"] = proximityStatePIC2;

  char message[512];
  serializeJson(jsonDoc, message);

  // Publish the message to the topic
  mqttClient.publish(TOPIC_CONFIG, message);
  Serial.println("Published current configuration:");
  Serial.println(message);
}

// Function to print the content of the JSON file on the serial monitor
void SensorsManager::readFileContent(){
  File file = SPIFFS.open(configFile, "r");
  if(!file) {
    Serial.println("Echec de l'ouverture du fichier pour la lecture");
    return;
  }

  Serial.println("contenu du fichier de configuration:");
  while (file.available()){
    Serial.write(file.read());
  }
  file.close();
  Serial.println("");
}

//PRIVATE FUNCTIONS

// Function to read a single byte from a register on a specified I2C bus
uint8_t SensorsManager::readRegister8(TwoWire &i2cBus, int slaveAddress, int reg) {
  i2cBus.beginTransmission(slaveAddress);
  i2cBus.write(reg);                  // Write the register address
  if (i2cBus.endTransmission(false) != 0) {
    Serial.println("Error: Failed to write register address");
    return 0;
  }
  i2cBus.requestFrom(slaveAddress, 1); // Request 1 byte
  return i2cBus.available() ? i2cBus.read() : 0;
}

// Function to read two bytes from a register on a specified I2C bus
uint16_t SensorsManager::readRegister16(TwoWire &i2cBus, int slaveAddress, int reg) {
  i2cBus.beginTransmission(slaveAddress);
  i2cBus.write(reg);                  // Write the register address
  if (i2cBus.endTransmission(false) != 0) {
    Serial.println("Error: Failed to write register address");
    return 0;
  }
  i2cBus.requestFrom(slaveAddress, 2); // Request 2 bytes
  uint8_t highByte = i2cBus.available() ? i2cBus.read() : 0;
  uint8_t lowByte = i2cBus.available() ? i2cBus.read() : 0;
  return (highByte << 8) | lowByte; // Combine high and low bytes
}

// Function to write one byte to a register on a specified I2C bus
bool SensorsManager::writeRegister8(TwoWire &i2cBus, int slaveAddress, int reg, uint8_t value) {
  i2cBus.beginTransmission(slaveAddress);
  i2cBus.write(reg);           // Write the register address
  i2cBus.write(value);         // Write the byte value  
  return (i2cBus.endTransmission() == 0);
}

// Function to write two bytes to a register on a specified I2C bus
bool SensorsManager::writeRegister16(TwoWire &i2cBus, int slaveAddress, int reg, uint16_t value) {
  i2cBus.beginTransmission(slaveAddress);
  i2cBus.write(reg);                  // Write the register address
  i2cBus.write(value >> 8);           // Write high byte
  i2cBus.write(value & 0xFF);         // Write low byte
  return (i2cBus.endTransmission() == 0);
}


// Function to read and display data from a specific slave on a specified I2C bus
void SensorsManager::readAndDisplayData(TwoWire &i2cBus, uint8_t slaveAddress, const char *busName) {
  Serial.print("Data from Slave Address: 0x");
  Serial.print(slaveAddress, HEX);
  Serial.print(" on ");
  Serial.println(busName);

  // Read the touch reset parameter (1 byte)
  uint8_t resetState = readRegister8(i2cBus, slaveAddress, REG_RESET_STATE);
  Serial.print("1. Touch Reset Parameter: ");
  Serial.println(resetState, HEX);

  // Read the touch state parameter (1 byte, bit field)
  uint8_t touchState = readRegister8(i2cBus, slaveAddress, REG_TOUCH_STATE);
  Serial.print("2. Touch State Parameter (Bit Field): 0x");
  Serial.print(touchState, HEX);
  Serial.print(" (");
  for (int b = 7; b >= 0; b--) {
    Serial.print((touchState >> b) & 1); // Print bit field
    if (b > 0) Serial.print(",");
  }
  Serial.println(")");

  // Read the touch deviation (2 bytes)
  uint16_t touchDeviation = readRegister16(i2cBus, slaveAddress, REG_TOUCH_DEVIATION);
  Serial.print("3. Touch Deviation (Touch Sensor): ");
  Serial.println((uint8_t)(touchDeviation >> 8)); // High byte
  Serial.print("4. Touch Deviation (Proximity Sensor): ");
  Serial.println((uint8_t)(touchDeviation & 0xFF)); // Low byte

  // Read the touch threshold (2 bytes)
  uint16_t touchThreshold = readRegister16(i2cBus, slaveAddress, REG_TOUCH_THRESHOLD);
  Serial.print("5. Touch Threshold (Touch Sensor): ");
  Serial.println((uint8_t)(touchThreshold >> 8)); // High byte
  Serial.print("6. Touch Threshold (Proximity Sensor): ");
  Serial.println((uint8_t)(touchThreshold & 0xFF)); // Low byte

  // Read the touch deviation scaling (2 bytes)
  uint16_t touchScaling = readRegister16(i2cBus, slaveAddress, REG_TOUCH_SCALING);
  Serial.print("7. Touch Deviation Scaling (Touch Sensor): ");
  Serial.println((uint8_t)(touchScaling >> 8)); // High byte
  Serial.print("8. Touch Deviation Scaling (Proximity Sensor): ");
  Serial.println((uint8_t)(touchScaling & 0xFF)); // Low byte

  Serial.println("-------------------------------");
}

// Function to update sensor configuration on the PIC via I2C
void SensorsManager::updateSensorConfiguration(int PIC_id, uint16_t scaling, uint16_t threshold) {
  uint16_t slaveAddress;
  TwoWire *bus;

  // Determine the slave address and bus
  if (PIC_id == 1) {
    slaveAddress = SLAVE1_ADDRESS;
    bus = &Wire;
  } else if (PIC_id == 2) {
    slaveAddress = SLAVE2_ADDRESS;
    bus = &Wire1;
  } else {
    Serial.println("Invalid PIC_id: Must be 1 or 2");
    return;
  }

  // Write the combined threshold value
  if (!writeRegister16(*bus, slaveAddress, REG_TOUCH_THRESHOLD, threshold)) {
    Serial.println("Failed to update threshold!");
  } else {
    Serial.print("Threshold updated for PIC ");
    Serial.println(PIC_id);
  }

  // Write the combined scaling value
  if (!writeRegister16(*bus, slaveAddress, REG_TOUCH_SCALING, scaling)) {
    Serial.println("Failed to update scaling!");
  } else {
    Serial.print("Scaling updated for PIC ");
    Serial.println(PIC_id);
  }

  // Read back and display the updated data for confirmation
  readAndDisplayData(*bus, slaveAddress, "Current configuration");
}

//Function to save the configuration to the JSON file
void SensorsManager::saveConfiguration(DynamicJsonDocument& doc){
  File file = SPIFFS.open(configFile, "w");
  if (file){
    serializeJson(doc, file);
    file.close();
    Serial.println("configuration sauvegardée.");
    readFileContent();
  }else{
    Serial.println("Echec de l'ouverture du fichier pour l'écriture.");
  }
}