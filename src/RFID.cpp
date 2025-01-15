#include "RFID.h"

RFIDManager::RFIDManager(PubSubClient &client, bool &configMode)
    : mqttClient(client), configMode(configMode), rfid(SS_PIN, RST_PIN) {}

void RFIDManager::initRFID() {
    SPI.begin(SCK_PIN, MISO_PIN, MOSI_PIN, SS_PIN);
    rfid.PCD_Init();
    loadConfiguration();
}

void RFIDManager::handleBadgeDetection() {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial())
        return;

    String uid = formatUID(rfid.uid.uidByte, rfid.uid.size);
    Serial.println(F("Badge détecté."));
    Serial.print(F("UID (hex): "));
    Serial.println(uid);

    if (configMode) {
        if (mqttClient.publish(mqttTopic, uid.c_str())) {
            Serial.println("UID publié sur le topic rfid/card.");
        } else {
            Serial.println("Échec de la publication MQTT.");
        }
    } 
    else {
        String action = "";
        if (isUIDRegistered(uid, action)) {
            Serial.println("Badge déjà enregistré.");
            DynamicJsonDocument actionMessage(512);
            actionMessage["id"] = uid;
            actionMessage["action"] = action;

            String jsonString;
            serializeJson(actionMessage, jsonString);
            mqttClient.publish(actionTopic, jsonString.c_str());
            Serial.println("Badge publié sur le topic rfid/action.");
        } else {
            Serial.println("Badge non enregistré.");
        }
        readFileContent();
    }

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}

void RFIDManager::enableConfigMode() {
    configMode = true;
    mqttClient.subscribe(configTopic);
}

void RFIDManager::handleMQTTMessage(String topic, DynamicJsonDocument &doc) {
    if (topic == configTopic) {
        handleConfigurationMessage(doc);
        mqttClient.unsubscribe(configTopic);
        configMode = false;
        Serial.println("Désabonné au topic: rfid/config");
        Serial.println("Mode configuration terminé");
    }
}

void RFIDManager::loadConfiguration() {
    if (!SPIFFS.exists(configFile)) return;

    File file = SPIFFS.open(configFile, "r");
    if (file) {
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, file);
        file.close();

        Serial.println("Configuration chargée:");
        serializeJsonPretty(doc, Serial);
        Serial.println();
    }
}

void RFIDManager::saveConfiguration(DynamicJsonDocument &doc) {
    File file = SPIFFS.open(configFile, "w");
    if (file) {
        serializeJson(doc, file);
        file.close();
        Serial.println("Configuration de la carte RFID sauvegardée.");
        readFileContent();
    } else {
        Serial.println("Échec de l'ouverture du fichier pour l'écriture.");
    }
}

bool RFIDManager::isUIDRegistered(String uid, String &action) {
    if (!SPIFFS.exists(configFile)) return false;

    File file = SPIFFS.open(configFile, "r");
    if (file) {
        DynamicJsonDocument doc(2048);
        deserializeJson(doc, file);
        file.close();

        JsonArray rfidArray = doc["rfid"].as<JsonArray>();
        for (JsonObject obj : rfidArray) {
            if (obj["cardID"].as<String>() == uid) {
                action = obj["action"].as<String>();
                return true;
            }
        }
    }
    return false;
}

void RFIDManager::handleConfigurationMessage(DynamicJsonDocument &doc) {
    String cardID = doc["id"];
    String action = doc["action"];

    DynamicJsonDocument configDoc(2048);
    if (SPIFFS.exists(configFile)) {
        File file = SPIFFS.open(configFile, "r");
        if (file) {
            deserializeJson(configDoc, file);
            file.close();
        }
    } else {
        configDoc["rfid"] = JsonArray();
    }

    JsonArray rfidArray = configDoc["rfid"].as<JsonArray>();
    bool found = false;
    for (JsonObject obj : rfidArray) {
        if (obj["cardID"].as<String>() == cardID) {
            obj["action"] = action;
            found = true;
            break;
        }
    }

    if (!found) {
        JsonObject newEntry = rfidArray.createNestedObject();
        newEntry["cardID"] = cardID;
        newEntry["action"] = action;
    }

    saveConfiguration(configDoc);
    Serial.println("Configuration mise à jour !");
    
}

String RFIDManager::formatUID(byte *buffer, byte bufferSize) {
    String uid = "";
    for (byte i = 0; i < bufferSize; i++) {
        uid += String(buffer[i], HEX);
    }
    return uid;
}

void RFIDManager::readFileContent() {
    File file = SPIFFS.open(configFile, "r");
    if (!file) {
        Serial.println("Échec de l'ouverture du fichier pour la lecture");
        return;
    }

    Serial.println("Contenu du fichier de configuration:");
    while (file.available()) {
        Serial.write(file.read());
    }
    file.close();
    Serial.println("");
}