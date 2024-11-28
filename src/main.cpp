#include <WiFi.h>
#include <SPIFFS.h>
#include <PubSubClient.h>
#include "RFID.h"

const char* ssid = "Galaxy S8 Dorian";          
const char* password = "dorianlb";  
const char* mqtt_server = "192.168.64.216"; 
const char* init_topic = "rfid/ini";       

WiFiClient espClient;          
PubSubClient client(espClient);

bool configMode = false; 


void setupWiFi();
void reconnectMQTT();
void checkWiFi();
void mqttCallback(char* topic, byte* payload, unsigned int length);

RFIDManager rfidManager(client, configMode); 

void setup() {
  Serial.begin(115200);  
  setupWiFi();          
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(mqttCallback);

  SPIFFS.begin(true); // Initialisation du système de fichiers
  rfidManager.initRFID(); // Initialisation RFID
}

void loop() {
  checkWiFi();
  if (!client.connected()) {
    reconnectMQTT();
  }
  client.loop();

  rfidManager.handleBadgeDetection(); 
}

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
      client.subscribe(init_topic); // S'abonner au topic "rfid/ini"
      Serial.println("Abonné au topic: rfid/ini");
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

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  DynamicJsonDocument doc(1024);
  deserializeJson(doc, payload, length);

  if (String(topic) == init_topic) {
    rfidManager.enableConfigMode(); // Active le mode configuration
    Serial.println("Mode configuration activé");
  }

  if (configMode) {
    rfidManager.handleMQTTMessage(String(topic), doc); 
  }
}