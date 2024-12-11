#include <WiFi.h>
#include <SPIFFS.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <Adafruit_CAP1188.h>
#include <ArduinoJson.h>

#define SS_PIN 21
#define CAP1188_CLK 18
const char* ssid = "Galaxy S8 Dorian";          
const char* password = "dorianlb";  
const char* mqtt_server = "192.168.20.196"; 

// MQTT topics
const char* topic_update = "sensor/update";
const char* topic_current_configuration = "sensor/current-configuration";

// CAP1188 configuration
Adafruit_CAP1188 cap1 = Adafruit_CAP1188(0x29); 
//Adafruit_CAP1188 cap2 = Adafruit_CAP1188(0x28);
WiFiClient espClient;
          
PubSubClient client(espClient);

void setupWiFi() {
  Serial.print("Connexion au Wi-Fi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.println("Wi-Fi connecté !");
  Serial.print("Adresse IP : ");
  Serial.println(WiFi.localIP());
}

void reconnectMQTT() {
  while (!client.connected()) {
    Serial.print("Connexion au serveur MQTT...");
    if (client.connect("ESP32Client")) {
      Serial.println("Connecté !");
      client.subscribe(topic_update); 
      Serial.println("Abonné au topic: sensor/update");
    } else {
      Serial.print("Échec, rc=");
      Serial.print(client.state());
      Serial.println(". Nouvelle tentative dans 5 secondes...");
      delay(5000);
    }
  }
}

void checkWiFi() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi déconnecté. Tentative de reconnexion...");
    setupWiFi();
  }
}

// Configure CAP1188 channel
void configureCAP(Adafruit_CAP1188* cap, int id, int sensitivity, int gain, int threshold) {
  // Example (replace with your actual configuration functions):
  Serial.printf("Configuring CAP: sensitivity=%d, gain=%d, threshold=%d\n", sensitivity, gain, threshold);
   // Sensitivity (write to register 0x1F)
  uint8_t currentSensitivityValue =cap->readRegister(0x1F);
  Serial.printf("Current sensitivity 0x%02X\n", currentSensitivityValue);
  Serial.printf("Current sensitivity %d\n", currentSensitivityValue);
  // currentSensitivityValue &= 0x8F; //Clear bits 4, 5 and 6
  // currentSensitivityValue |= (sensitivity & 0x07) << 4;
  cap->writeRegister(0x1F, sensitivity);
  Serial.printf("Sensitivity set to 0x%02X\n", sensitivity);
  Serial.printf("Sensitivity set to %d\n", sensitivity);


  // Gain (update bits 6 and 7 of register 0x00)
  uint8_t currentGainReg = cap->readRegister(0x00);
  Serial.printf("current gain 0x%02X \n", currentGainReg);
  Serial.printf("current gain %d \n", currentGainReg);
  currentGainReg &= 0x3F; // Clear bits 6 and 7
  currentGainReg |= (gain & 0x03) << 6; // Set gain bits
  cap->writeRegister(0x00, currentGainReg);
  Serial.printf("Gain set to 0x%02X (updated register 0x00)\n", currentGainReg);
  Serial.printf("Gain set to %d (updated register 0x00)\n", currentGainReg);


  // Thresholds (write to 0x30 and 0x31 for CS1 and CS2)
  uint8_t thresholdValue = static_cast<uint8_t>(threshold & 0xFF);
    if( id%2 != 0){
      cap->writeRegister(0x30, thresholdValue); // CS1 threshold
      Serial.printf("Threshold for CS1 set to 0x%02X\n", thresholdValue);
      Serial.printf("Threshold for CS1 set to %d\n", thresholdValue);

    }else{
      cap->writeRegister(0x31, thresholdValue); // CS2 threshold
      Serial.printf("Threshold for CS2 set to 0x%02X\n", thresholdValue);
      Serial.printf("Threshold for CS2 set to %d \n", thresholdValue);

    }


}

// Parse and handle update messages
void handleUpdateMessage(byte* payload, unsigned int length) {
  char message[length + 1];
  memcpy(message, payload, length);
  message[length] = '\0';

  DynamicJsonDocument doc(2048);
  DeserializationError error = deserializeJson(doc, message);

  if (error) {
    Serial.print("Failed to parse message: ");
    Serial.println(error.c_str());
    return;
  }

  int id = doc["id"];
  int sensitivity = doc["sensitivity"];
  int gain = doc["gain"];
  int threshold = doc["threshold"];
  Adafruit_CAP1188* cap = &cap1;
  //Adafruit_CAP1188* cap = (id <= 2) ? &cap1 : &cap2;
  configureCAP(cap, id, sensitivity, gain, threshold);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, topic_update) == 0) {
    handleUpdateMessage(payload, length);
  }
}

// Read and publish current configuration (delta values)
void publishCurrentConfiguration() {

  int delta1 = cap1.readRegister(0x10); 
  int delta2 = cap1.readRegister(0x11); 

  // int delta3 = cap2.readRegister(0x10);
  // int delta4 = cap2.readRegister(0x11);
  if(delta1>127){
    delta1 = delta1 - 255;
  }
  if(delta2>127){
    delta2 = delta2 - 255;
  }

  DynamicJsonDocument doc(2048);
  doc["delta1"] = delta1;
  doc["delta2"] = delta2;
  // doc["delta3"] = delta3;
  // doc["delta4"] = delta4;

  char buffer[256];
  size_t n = serializeJson(doc, buffer);

  client.publish(topic_current_configuration, buffer, n);

}


void setup() {
  Serial.begin(115200);  
  setupWiFi();          
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

   // Initialize CAP1188
  if (!cap1.begin() /*|| !cap2.begin()*/) {
    Serial.println("CAP1188 not detected. Check wiring!");
    while (1);
  }

  // Connect to MQTT
  reconnectMQTT();  
}

void loop() {
  checkWiFi();
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  // Publish sensor data 5 times per second
  static unsigned long lastMillis = 0;
  if (millis() - lastMillis >= 200) {
    lastMillis = millis();
    publishCurrentConfiguration();
  }
}


