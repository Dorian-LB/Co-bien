#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <FS.h>

// Wi-Fi credentials
const char* ssid = "Galaxy S8 Dorian";
const char* password = "dorianlb";

// MQTT broker details
const char* mqttServer = "192.168.228.196";
const int mqttPort = 1883;

WiFiClient espClient;
PubSubClient client(espClient);

// MQTT topics
#define TOPIC_CONFIG "sensor/current-configuration"
#define TOPIC_SENSOR_UPDATE "sensor/update"

/*TWO I2C BUSES*/
// I2C addresses for the PIC16LF1559 devices
#define SLAVE1_ADDRESS 0x28  // First PIC16LF1559 on I2C0 (Wire)
#define SLAVE2_ADDRESS 0x29  // Second PIC16LF1559 on I2C1 (Wire1)

// Register addresses
#define REG_RESET_STATE        0x00  // Touch reset parameter
#define REG_TOUCH_STATE        0x01  // Touch state parameter
#define REG_TOUCH_DEVIATION    0x10  // Touch deviation 
#define REG_TOUCH_THRESHOLD    0x30  // Touch threshold 
#define REG_TOUCH_SCALING      0x50  // Touch deviation scaling 

// Path for configuration file
const char* configFile = "/conf.json";


// Function to read a single byte from a register on a specified I2C bus
uint8_t readRegister8(TwoWire &i2cBus, int slaveAddress, int reg) {
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
uint16_t readRegister16(TwoWire &i2cBus, int slaveAddress, int reg) {
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
bool writeRegister8(TwoWire &i2cBus, int slaveAddress, int reg, uint16_t value) {
  i2cBus.beginTransmission(slaveAddress);
  i2cBus.write(reg);           // Write the register address
  i2cBus.write(value);         // Write the byte value  
  return (i2cBus.endTransmission() == 0);
}

// Function to write two bytes to a register on a specified I2C bus
bool writeRegister16(TwoWire &i2cBus, int slaveAddress, int reg, uint16_t value) {
  i2cBus.beginTransmission(slaveAddress);
  i2cBus.write(reg);                  // Write the register address
  i2cBus.write(value >> 8);           // Write high byte
  i2cBus.write(value & 0xFF);         // Write low byte
  return (i2cBus.endTransmission() == 0);
}


// Function to print the content of the JSON file on the serial monitor
void readFileContent(){
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

// Function to read and display data from a specific slave on a specified I2C bus
void readAndDisplayData(TwoWire &i2cBus, uint8_t slaveAddress, const char *busName) {
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
void updateSensorConfiguration(int sensorId, uint16_t scaling, uint16_t threpshold) {
  
  // Determine slave address, I2C bus, and specific sensor registers based on sensorId
  uint16_t slaveAddress;
  TwoWire *bus;

  if (sensorId == 1 || sensorId == 2) { // PIC1
    slaveAddress = SLAVE1_ADDRESS;
    bus = &Wire;
  } else if (sensorId == 3 || sensorId == 4) { // PIC2
    slaveAddress = SLAVE2_ADDRESS;
    bus = &Wire1;
 
  } else {
    Serial.println("Invalid sensorId: Must be 1, 2, 3, or 4");
    return;
  }
  if(sensorId%2 == 0){
    threshold = (threshold & 0x00FF);
    scaling = (scaling & 0x00FF);
  }else{
    threshold = (threshold & 0xFF00);
    scaling = (scaling & 0xFF00);
  }
  // Update threshold value
  if (!writeRegister8(*bus, slaveAddress, REG_TOUCH_THRESHOLD, threshold)) {
    Serial.println("Failed to update threshold!");
  } else {
    Serial.print("Threshold updated for Sensor ");
    Serial.println(sensorId);
  }

  // Update scaling value
  if (!writeRegister8(*bus, slaveAddress, REG_TOUCH_SCALING, scaling)) {
    Serial.println("Failed to update scaling!");
  } else {
    Serial.print("Scaling updated for Sensor ");
    Serial.println(sensorId);
  }
  readAndDisplayData(*bus, slaveAddress, "Current configuration");
}


// Function to read the sensor configuration from the JSON file
bool setupSensors() {
  DynamicJsonDocument doc(2048);
  if(SPIFFS.exists(configFile)) {
    File file = SPIFFS.open(configFile, "r");
    if (file) {
        deserializeJson(doc,file);
      }
  }
  JsonArray sensorsArray = doc["sensors"].as<JsonArray>();
  for (JsonObject obj : sensorsArray) {
      updateSensorConfiguration(obj["sensor_id"], obj["scaling"], obj["threshold"]);
    }
  return true;
  }



// Function to send current sensors deviations via MQTT
void sendSensorDeviation() {
  StaticJsonDocument<512> jsonDoc; // Adjust size based on number of sensors

  // Read deviation parameters for the first PIC (touch and proximity sensors)
  uint16_t deviationPIC1 = readRegister16(Wire, SLAVE1_ADDRESS, REG_TOUCH_DEVIATION);
  uint8_t touchDeviationPIC1 = (deviationPIC1 >> 8) & 0xFF;  // High byte
  uint8_t proximityDeviationPIC1 = deviationPIC1 & 0xFF;     // Low byte

  // Read deviation parameters for the second PIC (touch and proximity sensors)
  uint16_t deviationPIC2 = readRegister16(Wire1, SLAVE2_ADDRESS, REG_TOUCH_DEVIATION);
  uint8_t touchDeviationPIC2 = (deviationPIC2 >> 8) & 0xFF;  // High byte
  uint8_t proximityDeviationPIC2 = deviationPIC2 & 0xFF;     // Low byte

  // Add data to JSON
  jsonDoc["PIC1"]["touchDeviation"] = touchDeviationPIC1;
  jsonDoc["PIC1"]["proximityDeviation"] = proximityDeviationPIC1;
  jsonDoc["PIC2"]["touchDeviation"] = touchDeviationPIC2;
  jsonDoc["PIC2"]["proximityDeviation"] = proximityDeviationPIC2;

  char message[512];
  serializeJson(jsonDoc, message);

  // Publish the message to the topic
  client.publish(TOPIC_CONFIG, message);
  Serial.println("Published current configuration:");
  Serial.println(message);
}
//Function to save the configuration to the JSON file
void saveConfiguration(DynamicJsonDocument& doc){
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

// Function to store the configuration changes in a doc file
void handleSensorConfigurationMessage(DynamicJsonDocument& message) {
  int sensor_id = message["sensorId"];
  int scaling = message["scaling"];
  int threshold = message["threshold"];

  DynamicJsonDocument doc(2048);
  if(SPIFFS.exists(configFile)) {
    File file = SPIFFS.open(configFile, "r");
    if (file){
      deserializeJson(doc, file);
      file.close();
    }
  }
  Serial.println(sensor_id);
  JsonArray sensorsArray = doc["sensors"].as<JsonArray>();
  for (JsonObject obj : sensorsArray){
    if (obj["sensor_id"] == sensor_id){
      Serial.println(sensor_id);
      Serial.println(threshold);
      Serial.println(scaling);
      obj["threshold"] = threshold;
      obj["scaling"] = scaling;
      Serial.println(int(obj["threshold"]));
      Serial.println(int(obj["scaling"]));
      break;
    }
  }
  saveConfiguration(doc);
   updateSensorConfiguration(sensor_id, scaling, threshold);
}

// Callback function for incoming MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  //Convert payload to string
  String message = "";
  for (unsigned int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.print("received message: ");
  Serial.println(message);

  //Parse JSON
  DynamicJsonDocument configDoc(2048);
  DeserializationError error = deserializeJson(configDoc, message);

  if (strcmp(topic, TOPIC_SENSOR_UPDATE) == 0) {
    Serial.println("topic verification");
    handleSensorConfigurationMessage(configDoc);
  }
}

// Function to connect to Wi-Fi
void connectToWiFi() {
  Serial.print("Connecting to Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println("Connected to Wi-Fi!");
}

// Function to connect to MQTT broker
void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected!");
      client.subscribe(TOPIC_SENSOR_UPDATE); // Subscribe to the update topic
    } else {
      Serial.print("Failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);       // Initialize Serial Monitor

    // Initialize both I2C buses
  Wire.begin(21, 22);         // SDA, SCL pins for Wire (I2C0)
  Wire1.begin(25, 26);        // SDA, SCL pins for Wire1 (I2C1)
  Wire.setClock(100000);      // Set I2C clock speed for Wire
  Wire1.setClock(100000);     // Set I2C clock speed for Wire1

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialization failed");
    return;
  }else{
    Serial.println("SPIFFS mounted successfully");
  }
  readFileContent();
  // Load configuration from file
  if (!setupSensors()) {
    Serial.println("Using default configuration");
  } else{
    Serial.println("Using configuration from conf.json file");
  }



   // Initialize Wi-Fi and MQTT
  connectToWiFi();
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();

  // Send sensor configuration periodically
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis >= 200) { // 5 times per second
    sendSensorDeviation();
    lastMillis = millis();
  }

  // // Read and display data from the first PIC16LF1559 on Wire (I2C0)
  // readAndDisplayData(Wire, SLAVE1_ADDRESS, "Cancel");

  // // Read and display data from the second PIC16LF1559 on Wire1 (I2C1)
  // readAndDisplayData(Wire1, SLAVE2_ADDRESS, "Comfirm");

  // delay(1000); // Delay between readings
}

/*MULTIPLE SLAVE ADRESSES & ONE I2C BUS*/
// // I2C addresses for the PIC16LF1559 devices
// #define SLAVE1_ADDRESS 0x28  // First PIC16LF1559
// #define SLAVE2_ADDRESS 0x29  // Second PIC16LF1559

// // Register addresses
// #define REG_RESET_STATE        0x00  // Touch reset parameter
// #define REG_TOUCH_STATE        0x01  // Touch state parameter (bit field)
// #define REG_TOUCH_DEVIATION    0x10  // Touch deviation (2 bytes)
// #define REG_TOUCH_THRESHOLD    0x30  // Touch threshold (2 bytes)
// #define REG_TOUCH_SCALING      0x50  // Touch deviation scaling (2 bytes)

// // Function to read a single byte from a register of a specific slave
// uint8_t readRegister8(int slaveAddress, int reg) {
//   Wire.beginTransmission(slaveAddress);
//   Wire.write(reg);                  // Write the register address
//   if (Wire.endTransmission(false) != 0) {
//     Serial.println("Error: Failed to write register address");
//     return 0;
//   }
//   Wire.endTransmission(false);      // Send stop condition after address
//   Wire.requestFrom(slaveAddress, 1); // Request 1 byte
//   return Wire.available() ? Wire.read() : 0;
// }

// // Function to read two bytes from a register of a specific slave
// uint16_t readRegister16(int slaveAddress, int reg) {
//   Wire.beginTransmission(slaveAddress);
//   Wire.write(reg);                  // Write the register address
//   if (Wire.endTransmission(false) != 0) {
//     Serial.println("Error: Failed to write register address");
//     return 0;
//   }
//   Wire.endTransmission(false);      // Send stop condition after address
//   Wire.requestFrom(slaveAddress, 2); // Request 2 bytes
//   uint8_t highByte = Wire.available() ? Wire.read() : 0;
//   uint8_t lowByte = Wire.available() ? Wire.read() : 0;
//   return (highByte << 8) | lowByte; // Combine high and low bytes
// }

// // Function to read and display data from a specific slave
// void readAndDisplayData(uint8_t slaveAddress) {
//   Serial.print("Data from Slave Address: 0x");
//   Serial.println(slaveAddress, HEX);

//   // Read the touch reset parameter (1 byte)
//   uint8_t resetState = readRegister8(slaveAddress, REG_RESET_STATE);
//   Serial.print("1. Touch Reset Parameter: ");
//   Serial.println(resetState, HEX);

//   // Read the touch state parameter (1 byte, bit field)
//   uint8_t touchState = readRegister8(slaveAddress, REG_TOUCH_STATE);
//   Serial.print("2. Touch State Parameter (Bit Field): 0x");
//   Serial.print(touchState, HEX);
//   Serial.print(" (");
//   for (int b = 7; b >= 0; b--) {
//     Serial.print((touchState >> b) & 1); // Print bit field
//     if (b > 0) Serial.print(",");
//   }
//   Serial.println(")");

//   // Read the touch deviation (2 bytes)
//   uint16_t touchDeviation = readRegister16(slaveAddress, REG_TOUCH_DEVIATION);
//   Serial.print("3. Touch Deviation (Touch Sensor): ");
//   Serial.println((uint8_t)(touchDeviation >> 8)); // High byte
//   Serial.print("4. Touch Deviation (Proximity Sensor): ");
//   Serial.println((uint8_t)(touchDeviation & 0xFF)); // Low byte

//   // Read the touch threshold (2 bytes)
//   uint16_t touchThreshold = readRegister16(slaveAddress, REG_TOUCH_THRESHOLD);
//   Serial.print("5. Touch Threshold (Touch Sensor): ");
//   Serial.println((uint8_t)(touchThreshold >> 8)); // High byte
//   Serial.print("6. Touch Threshold (Proximity Sensor): ");
//   Serial.println((uint8_t)(touchThreshold & 0xFF)); // Low byte

//   // Read the touch deviation scaling (2 bytes)
//   uint16_t touchScaling = readRegister16(slaveAddress, REG_TOUCH_SCALING);
//   Serial.print("7. Touch Deviation Scaling (Touch Sensor): ");
//   Serial.println((uint8_t)(touchScaling >> 8)); // High byte
//   Serial.print("8. Touch Deviation Scaling (Proximity Sensor): ");
//   Serial.println((uint8_t)(touchScaling & 0xFF)); // Low byte

//   Serial.println("-------------------------------");
// }

// void setup() {
//   Serial.begin(115200);       // Initialize Serial Monitor
//   Wire.begin();               // Initialize I2C as Master
//   Wire.setClock(100000);      // Set I2C clock speed (100kHz)

// }

// void loop() {
//   // Read and display data from the first PIC16LF1559
//   readAndDisplayData(SLAVE1_ADDRESS);
//   delay(10);
//   // Read and display data from the second PIC16LF1559 
//   readAndDisplayData(SLAVE2_ADDRESS);

//   delay(1000); // Delay between readings
// }



/*SINGLE SLAVE ADDRESS & MULTIPLEXER*/
// #define TCA9548A_ADDRESS 0x70  // Address of the TCA9548A multiplexer

// // Register addresses
// #define REG_RESET_STATE        0x00  // Touch reset parameter
// #define REG_TOUCH_STATE        0x01  // Touch state parameter (bit field)
// #define REG_TOUCH_DEVIATION    0x10  // Touch deviation (2 bytes)
// #define REG_TOUCH_THRESHOLD    0x30  // Touch threshold (2 bytes)
// #define REG_TOUCH_SCALING      0x50  // Touch deviation scaling (2 bytes)

// // Function to select TCA9548A channel
// void selectMultiplexerChannel(uint8_t channel) {
//   Wire.beginTransmission(TCA9548A_ADDRESS);
//   Wire.write(1 << channel); // Select the channel (0-7)
//   Wire.endTransmission();
// }

// // Function to read a single byte from a register
// uint8_t readRegister8(uint8_t reg) {
//   Wire.beginTransmission(0x28);  // Address of the PIC
//   Wire.write(reg);               // Write the register address
//   Wire.endTransmission(false);   // No stop condition
//   Wire.requestFrom(0x28, 1);     // Request 1 byte
//   return Wire.available() ? Wire.read() : 0;
// }

// // Function to read two bytes from a register
// uint16_t readRegister16(uint8_t reg) {
//   Wire.beginTransmission(0x28);  // Address of the PIC
//   Wire.write(reg);               // Write the register address
//   Wire.endTransmission(false);   // No stop condition
//   Wire.requestFrom(0x28, 2);     // Request 2 bytes
//   uint8_t highByte = Wire.available() ? Wire.read() : 0;
//   uint8_t lowByte = Wire.available() ? Wire.read() : 0;
//   return (highByte << 8) | lowByte; // Combine high and low bytes
// }

// // Function to process and print data for a specific channel
// void processPICChannel(uint8_t channel) {
//   selectMultiplexerChannel(channel); // Select the TCA channel
//   Serial.print("Data from PIC on TCA Channel: ");
//   Serial.println(channel);

//   // Read the touch reset parameter (1 byte)
//   uint8_t resetState = readRegister8(REG_RESET_STATE);
//   Serial.print("1. Touch Reset Parameter: ");
//   Serial.println(resetState, HEX);

//   // Read the touch state parameter (1 byte, bit field)
//   uint8_t touchState = readRegister8(REG_TOUCH_STATE);
//   Serial.print("2. Touch State Parameter (Bit Field): 0x");
//   Serial.print(touchState, HEX);
//   Serial.print(" (");
//   for (int b = 7; b >= 0; b--) {
//     Serial.print((touchState >> b) & 1); // Print bit field
//     if (b > 0) Serial.print(",");
//   }
//   Serial.println(")");

//   // Read the touch deviation (2 bytes)
//   uint16_t touchDeviation = readRegister16(REG_TOUCH_DEVIATION);
//   Serial.print("3. Touch Deviation (Touch Sensor): ");
//   Serial.println((uint8_t)(touchDeviation >> 8)); // High byte
//   Serial.print("4. Touch Deviation (Proximity Sensor): ");
//   Serial.println((uint8_t)(touchDeviation & 0xFF)); // Low byte

//   // Read the touch threshold (2 bytes)
//   uint16_t touchThreshold = readRegister16(REG_TOUCH_THRESHOLD);
//   Serial.print("5. Touch Threshold (Touch Sensor): ");
//   Serial.println((uint8_t)(touchThreshold >> 8)); // High byte
//   Serial.print("6. Touch Threshold (Proximity Sensor): ");
//   Serial.println((uint8_t)(touchThreshold & 0xFF)); // Low byte

//   // Read the touch deviation scaling (2 bytes)
//   uint16_t touchScaling = readRegister16(REG_TOUCH_SCALING);
//   Serial.print("7. Touch Deviation Scaling (Touch Sensor): ");
//   Serial.println((uint8_t)(touchScaling >> 8)); // High byte
//   Serial.print("8. Touch Deviation Scaling (Proximity Sensor): ");
//   Serial.println((uint8_t)(touchScaling & 0xFF)); // Low byte

//   Serial.println("-------------------------------");
// }

// void setup() {
//   Serial.begin(115200);       // Initialize Serial Monitor
//   Wire.begin();               // Initialize I2C as Master
//   Wire.setClock(100000);      // Set I2C clock speed (100kHz)
// }

// void loop() {
//   processPICChannel(1); // Process PIC on TCA channel 1
//   processPICChannel(0); // Process PIC on TCA channel 0
//   delay(1000);          // Delay between readings
// }

/*I2C Scanner*/
// void setup() {
//     Wire.begin();
//     Serial.begin(115200);
//     Serial.println("Scanning...");
//     for (uint8_t addr = 1; addr < 127; addr++) {
//         Wire.beginTransmission(addr);
//         if (Wire.endTransmission() == 0) {
//             Serial.print("Found device at 0x");
//             Serial.println(addr, HEX);
//         }
//     }
// }
// void loop() {}