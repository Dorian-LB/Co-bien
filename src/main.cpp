#include <Wire.h>
#include <SPI.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <FS.h>
#include <FastLED.h>

#include "RFID.h"
#include "LEDs.h"
#include "sensors.h"

#define LEDS_CONFIG_TOPIC "ledstrip/config"
#define LEDS_UPDATE_TOPIC "ledstrip/update"
#define RFID_INI_TOPIC "rfid/ini"

// #define NUM_LEDS 18
//Nombre de leds pour 3 boutons
#define NUM_LEDS 27
// //Nombre de leds pour 3 boutons
// #define NUM_LEDS 36

#define DATA_PIN_GROUP_LED1 2
#define DATA_PIN_GROUP_LED2 13
#define DATA_PIN_GROUP_LED3 14
#define DATA_PIN_GROUP_LED4 0

#define FADING_BLINK_SPEED 50

// Wi-Fi credentials
const char* ssid = "Galaxy S8 Dorian";
const char* password = "dorianlb";

// MQTT broker details
const char* mqttServer = "192.168.172.196";
const int mqttPort = 1883;

// Path for configuration file
const char* configFile = "/conf.json";

bool configMode = false; 

WiFiClient espClient;
PubSubClient client(espClient);
SensorsManager sensors(client);
RFIDManager rfidManager(client, configMode); 


CRGB leds[NUM_LEDS];
int group1[] = {0,1,2,3,4,5,6,7,8};
int group2[] = {9,10,11,12,13,14,15,16,17};   
int group3[] = {18,19,20,21,22,23,24,25,26};
// int group4[] = {27,28,29,30,31,32,33,34,35};

// // Calculate the number of elements
// int numberOfElementsGroup1 = sizeof(group1) / sizeof(group1[0]);

LEDGroup ledGroup1(group1, sizeof(group1) / sizeof(group1[0]));
LEDGroup ledGroup2(group2, sizeof(group2) / sizeof(group2[0]));
LEDGroup ledGroup3(group3, sizeof(group3) / sizeof(group3[0]));
// LEDGroup ledGroup4(group4, sizeof(group4) / sizeof(group4[0]));

void setupWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
}

// Function to connect to MQTT broker
void connectToMQTT() {
  while (!client.connected()) {
    Serial.print("Connecting to MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connected!");
      client.subscribe(TOPIC_SENSOR_UPDATE); // Subscribe to the update topic
      client.subscribe(LEDS_CONFIG_TOPIC);
      client.subscribe(LEDS_UPDATE_TOPIC);
      client.subscribe(RFID_INI_TOPIC); // S'abonner au topic "rfid/ini"
    } else {
      Serial.print("Failed with state ");
      Serial.println(client.state());
      delay(2000);
    }
  }
}

void readFileContent() {
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


void saveConfiguration(DynamicJsonDocument& doc) {
  File file = SPIFFS.open(configFile, "w");
  if (file) {
    serializeJson(doc, file);
    file.close();
    Serial.println("Configuration des LEDs sauvegardée dans conf.json.");
    readFileContent();
  } else {
    Serial.println("Échec de l'ouverture du fichier pour l'écriture.");
  }
}

void handleLedConfigurationMessage(char* topic, DynamicJsonDocument& message) {
    // Extract JSON values
    int group = message["group"];
    String mode = message["mode"];
    String colorHex = message["color"];
    uint8_t intensity = message["intensity"];

    if (strcmp(topic, LEDS_CONFIG_TOPIC) == 0){
      // ---- Add the message to the conf.json file------
      DynamicJsonDocument doc(2048);
      if (SPIFFS.exists(configFile)) {
        File file = SPIFFS.open(configFile, "r");
        if (file) {
          deserializeJson(doc, file);
          file.close();
        }
      } 
      JsonArray ledsArray = doc["led_groups"].as<JsonArray>();
      for (JsonObject obj : ledsArray) {
        if (obj["group_id"] == group) {
          obj["mode"] = mode;
          obj["color"] = colorHex;
          obj["intensity"] = intensity;
          break;
        }
      }
      saveConfiguration(doc);
      Serial.println("Configuration mise à jour !");
    }

    // Convert color from hex string to CRGB
    uint32_t colorValue = strtoul(colorHex.c_str() + 1, NULL, 16);
    CRGB color = CRGB((colorValue >> 16) & 0xFF, (colorValue >> 8) & 0xFF, colorValue & 0xFF);


    // Set mode for LED group
    LEDMode ledMode;

    if (mode == "FADING_BLINK") {
        ledMode = FADING_BLINK;
    } else if (mode == "BLINK") {
        ledMode = BLINK;
    } else if (mode == "ON") {
        ledMode = ON;
    } else if (mode == "OFF") {
        ledMode = OFF;
    } else {
        Serial.println("Invalid mode received. Defaulting to OFF.");
        ledMode = OFF; // Default mode in case of an invalid string
    }

    // Configure the appropriate LED group
    switch (group) {
        case 1:
            if(ledMode == FADING_BLINK){
            ledGroup1.configure(color,intensity,ledMode, FADING_BLINK_SPEED);
            }else{ledGroup1.configure(color,intensity,ledMode);
            }
            break;
        case 2:
            if(ledMode == FADING_BLINK){
            ledGroup2.configure(color,intensity,ledMode, FADING_BLINK_SPEED);
            }else{ledGroup2.configure(color,intensity,ledMode);
            }
            break;
        case 3:
             if(ledMode == FADING_BLINK){
            ledGroup3.configure(color,intensity,ledMode, FADING_BLINK_SPEED);
            }else{ledGroup3.configure(color,intensity,ledMode);
            }
            break;
        // case 4:
        //      if(ledMode == FADING_BLINK){
        //     ledGroup4.configure(color,intensity,ledMode, FADING_BLINK_SPEED);
        //     }else{ledGroup4.configure(color,intensity,ledMode);
        //     }
        //     break;
        // default:
        //     Serial.println("Invalid group number.");
        //     break;
    }
  }

void callback(char* topic, byte* payload, unsigned int length) {
  // Convert payload to string
    String message = "";
    for (unsigned int i = 0; i < length; i++) {
        message += (char)payload[i];
    }
    Serial.print("Received message: ");
    Serial.println(message);

    // Parse JSON
    DynamicJsonDocument configDoc(2048);
    DeserializationError error = deserializeJson(configDoc, message);
    // if (error) {
    //     Serial.print("JSON parsing failed: ");
    //     Serial.println(error.c_str());
    //     return;
    // }
    if (String(topic) == RFID_INI_TOPIC) {
    rfidManager.enableConfigMode(); // Active le mode configuration
    Serial.println("Mode configuration activé");
  }

  if (configMode) {
    rfidManager.handleMQTTMessage(String(topic), configDoc); 
  }
      // ----Configuration of the LEDs------
  if (strcmp(topic, LEDS_CONFIG_TOPIC) == 0 || strcmp(topic, LEDS_UPDATE_TOPIC) == 0){
    handleLedConfigurationMessage(topic, configDoc);
  }


  if (strcmp(topic, TOPIC_SENSOR_UPDATE) == 0) {
    Serial.println("topic verification");
    sensors.handleSensorConfigurationMessage(configDoc);
  }
}
void setupLEDs() {
    
    DynamicJsonDocument doc(2048);
    if (SPIFFS.exists(configFile)) {
      File file = SPIFFS.open(configFile, "r");
      if (file) {
        deserializeJson(doc, file);
        file.close();
      }
    } 

    //Créer les leds
    FastLED.addLeds<SK6812, DATA_PIN_GROUP_LED1, GRB>(leds,group1[0], sizeof(group1) / sizeof(group1[0]));
    FastLED.addLeds<SK6812, DATA_PIN_GROUP_LED2, GRB>(leds,group2[0], sizeof(group2) / sizeof(group2[0]));
    FastLED.addLeds<SK6812, DATA_PIN_GROUP_LED3, GRB>(leds,group3[0], sizeof(group3) / sizeof(group3[0]));
    // FastLED.addLeds<SK6812, DATA_PIN_GROUP_LED4, GRB>(leds,group4[0], sizeof(group4) / sizeof(group4[0]));
    
    JsonArray ledsArray = doc["led_groups"].as<JsonArray>();
    for (JsonObject obj : ledsArray) {
        // Convert color from hex string to CRGB
        String colorHex = obj["color"];
        uint32_t colorValue = strtoul(colorHex.c_str() + 1, NULL, 16);
        CRGB color = CRGB((colorValue >> 16) & 0xFF, (colorValue >> 8) & 0xFF, colorValue & 0xFF);

        // Set mode for LED group
        LEDMode ledMode;

        if (obj["mode"] == "FADING_BLINK") {
            ledMode = FADING_BLINK;
        } else if (obj["mode"] == "BLINK") {
            ledMode = BLINK;
        } else if (obj["mode"] == "ON") {
            ledMode = ON;
        } else if (obj["mode"] == "OFF") {
            ledMode = OFF;
        } else {
            Serial.println("Invalid mode received. Defaulting to OFF.");
            ledMode = OFF; // Default mode in case of an invalid string
        }
        
        if (obj["group_id"] == 1) {

            if(ledMode == FADING_BLINK){
                ledGroup1.configure(color,obj["intensity"],ledMode,FADING_BLINK_SPEED);
            }else{
                ledGroup1.configure(color,obj["intensity"],ledMode);
            }

            }
        else if (obj["group_id"] == 2) {

            if(ledMode == FADING_BLINK){
                ledGroup2.configure(color,obj["intensity"],ledMode,FADING_BLINK_SPEED);
            }else{
                ledGroup2.configure(color,obj["intensity"],ledMode);

            }   
        }
        else if (obj["group_id"] == 3) {

            if(ledMode == FADING_BLINK){
                ledGroup3.configure(color,obj["intensity"],ledMode,FADING_BLINK_SPEED);
            }else{
                ledGroup3.configure(color,obj["intensity"],ledMode);

            }   
        }
        // else if (obj["group_id"] == 4) {

        //     if(ledMode == FADING_BLINK){
        //         ledGroup4.configure(color,obj["intensity"],ledMode,FADING_BLINK_SPEED);
        //     }else{
        //         ledGroup4.configure(color,obj["intensity"],ledMode);

        //     }   
        // }
    } 
}

void setup() {
  Serial.begin(115200);  

  // Initialize SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("SPIFFS initialization failed");
    return;
  }else{
    Serial.println("SPIFFS mounted successfully");
  }
  readFileContent();
  
  setupWiFi();          
  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);

  sensors.initializeI2C();   // Initialize both I2C buses
  sensors.setupSensors();
  rfidManager.initRFID(); // Initialisation RFID
  setupLEDs();

}

void loop() {
  ledGroup1.update();
  ledGroup2.update();
  ledGroup3.update();
  // ledGroup4.update();

  if (!client.connected()) {
    connectToMQTT();
  }
  client.loop();
  
  rfidManager.handleBadgeDetection(); 

  // Send sensor configuration periodically
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis >= 333) { // 3 times per second
    sensors.sendSensorDeviation();
    lastMillis = millis();
  }

}