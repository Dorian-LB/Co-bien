#ifndef SENSORS_H
#define SENSORS_H

#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <FS.h>

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

class SensorsManager {
    public:
        SensorsManager(PubSubClient &client);

        void initializeI2C();
        bool setupSensors();
        void handleSensorConfigurationMessage(DynamicJsonDocument& message);
        void sendSensorDeviation();
        void readFileContent();

    private:
        uint8_t readRegister8(TwoWire &i2cBus, int slaveAddress, int reg);
        uint16_t readRegister16(TwoWire &i2cBus, int slaveAddress, int reg);
        bool writeRegister8(TwoWire &i2cBus, int slaveAddress, int reg, uint8_t value);
        bool writeRegister16(TwoWire &i2cBus, int slaveAddress, int reg, uint16_t value);
        void readAndDisplayData(TwoWire &i2cBus, uint8_t slaveAddress, const char *busName);
        void updateSensorConfiguration(int PIC_id, uint16_t scaling, uint16_t threshold);
        void saveConfiguration(DynamicJsonDocument& doc);

        PubSubClient &mqttClient;
        const char* configFile = "/conf.json";
};

#endif