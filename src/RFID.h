#ifndef RFID_H
#define RFID_H

#include <SPI.h>
#include <MFRC522.h>
#include <FS.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>

#define SS_PIN 21
#define RST_PIN 14
#define SCK_PIN 18
#define MOSI_PIN 23
#define MISO_PIN 19

class RFIDManager {
public:
    RFIDManager(PubSubClient &client, bool &configMode);

    void initRFID();
    void handleBadgeDetection();
    void enableConfigMode();
    bool isConfigModeActive();
    void handleMQTTMessage(String topic, DynamicJsonDocument &doc);

private:
    void loadConfiguration();
    void saveConfiguration(DynamicJsonDocument &doc);
    bool isUIDRegistered(String uid, String &link);
    void handleConfigurationMessage(DynamicJsonDocument &doc);
    String formatUID(byte *buffer, byte bufferSize);
    void readFileContent();

    MFRC522 rfid;
    PubSubClient &mqttClient;
    bool &configMode; // Référence vers le mode configuration
    const char* configFile = "/conf.json";
    const char* mqttTopic = "rfid/card";
    const char* configTopic = "rfid/config";
    const char* actionTopic = "rfid/action";
};

#endif