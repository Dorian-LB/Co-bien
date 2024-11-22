#include <FastLED.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "LEDs.h"

// WiFi and MQTT Settings
#define WIFI_SSID "Galaxy S8 Dorian"
#define WIFI_PASSWORD "dorianlb"
#define MQTT_SERVER "192.168.250.196"
#define MQTT_PORT 1883
#define MQTT_TOPIC "ledControl/group"

WiFiClient espClient;
PubSubClient client(espClient);

#define NUM_LEDS 8
#define DATA_PIN 16
#define FADING_BLINK_SPEED 50

CRGB leds[NUM_LEDS];

int group1[] = {0, 1};
int group2[] = {2, 3};
int group3[] = {4, 5};
int group4[] = {6, 7};

LEDGroup ledGroup1(group1, 2);
LEDGroup ledGroup2(group2, 2);
LEDGroup ledGroup3(group3, 2);
LEDGroup ledGroup4(group4, 2);

void setupWiFi() {
    Serial.print("Connecting to WiFi...");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.print(".");
    }
    Serial.println("\nWiFi connected.");
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
    DynamicJsonDocument doc(256);
    DeserializationError error = deserializeJson(doc, message);
    if (error) {
        Serial.print("JSON parsing failed: ");
        Serial.println(error.c_str());
        return;
    }

    // Extract JSON values
    int group = doc["group"];
    String mode = doc["mode"];
    String colorHex = doc["color"];
    uint8_t intensity = doc["intensity"];

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

    // Update the appropriate LED group
    switch (group) {
        case 1:
            ledGroup1.setColor(color);
            ledGroup1.setIntensity(intensity);

            if(ledMode == FADING_BLINK){
            ledGroup1.setMode(ledMode, FADING_BLINK_SPEED);
            }else{ledGroup1.setMode(ledMode);
            }
            break;
        case 2:
            ledGroup2.setColor(color);
            ledGroup2.setIntensity(intensity);
            if(ledMode == FADING_BLINK){
            ledGroup2.setMode(ledMode, FADING_BLINK_SPEED);
            }else{ledGroup2.setMode(ledMode);
            }
            break;
        case 3:
            ledGroup3.setColor(color);
            ledGroup3.setIntensity(intensity);
            if(ledMode == FADING_BLINK){
            ledGroup3.setMode(ledMode, FADING_BLINK_SPEED);
            }else{ledGroup3.setMode(ledMode);
            }
            break;
        case 4:
            ledGroup4.setColor(color);
            ledGroup4.setIntensity(intensity);
            if(ledMode == FADING_BLINK){
            ledGroup4.setMode(ledMode, FADING_BLINK_SPEED);
            }else{ledGroup4.setMode(ledMode);
            }
            break;
        default:
            Serial.println("Invalid group number.");
            break;
    }
}

void reconnect() {
    while (!client.connected()) {
        Serial.print("Connecting to MQTT...");
        if (client.connect("LEDController")) {
            Serial.println("connected.");
            client.subscribe(MQTT_TOPIC);
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" retrying in 5 seconds...");
            delay(5000);
        }
    }
}
void setup() {
    Serial.begin(115200);

    // WiFi and MQTT setup
    setupWiFi();
    client.setServer(MQTT_SERVER, MQTT_PORT);
    client.setCallback(callback);

    FastLED.addLeds<SK6812, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.clear();

    ledGroup1.setColor(CRGB::Red);
    ledGroup2.setColor(CRGB::Green);
    ledGroup3.setColor(CRGB::Blue);
    ledGroup4.setColor(CRGB::Yellow);

    ledGroup1.setMode(FADING_BLINK, FADING_BLINK_SPEED); // Fast fading blink
    ledGroup2.setMode(BLINK);           // BLINK ON/OFF mode
    ledGroup3.setMode(ON); // ONN
    ledGroup4.setMode(OFF);      // OFF
}

void loop() {
    if (!client.connected()) {
        reconnect();
    }
    client.loop();

    ledGroup1.update();
    ledGroup2.update();
    ledGroup3.update();
    ledGroup4.update();
}