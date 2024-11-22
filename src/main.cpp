#include <FastLED.h>

#define NUM_LEDS 8 // Total number of LEDs
#define DATA_PIN 16 // Pin to which the LED strip is connected

CRGB leds[NUM_LEDS]; // Array to hold the LED data

// Define the LEDGroup class
class LEDGroup {
private:
    int *indices;   // Indices of LEDs in the group
    int size;       // Number of LEDs in the group
    CRGB color;     // Current color
    uint8_t intensity; // Intensity (brightness)
    bool blink;     // Blink state
    bool state;     // Current ON/OFF state for blinking
    unsigned long lastUpdate; // Time of the last blink update
    unsigned int blinkInterval; // Blink interval in milliseconds

public:
    // Constructor
    LEDGroup(int *ledIndices, int ledCount, unsigned int interval = 500)
        : indices(ledIndices), size(ledCount), color(CRGB::Black), intensity(255),
          blink(false), state(true), lastUpdate(0), blinkInterval(interval) {}

    // Set the color of the group
    void setColor(CRGB newColor) {
        color = newColor;
        updateLEDs();
    }

    // Set the intensity (brightness) of the group
    void setIntensity(uint8_t newIntensity) {
        intensity = newIntensity;
        FastLED.setBrightness(intensity);
        updateLEDs();
    }

    // Enable or disable blinking
    void setBlink(bool enable, unsigned int interval = 500) {
        blink = enable;
        blinkInterval = interval;
    }

    // Update the LEDs (called in the main loop)
    void update() {
        if (blink) {
            unsigned long currentTime = millis();
            if (currentTime - lastUpdate >= blinkInterval) {
                state = !state;
                lastUpdate = currentTime;
                updateLEDs();
            }
        }
    }

    // Helper to update the LED array
    void updateLEDs() {
        for (int i = 0; i < size; i++) {
            leds[indices[i]] = state ? color : CRGB::Black;
        }
        FastLED.show();
    }
};

// Define groups
int group1[] = {0, 1};
int group2[] = {2, 3};
int group3[] = {4, 5};
int group4[] = {6, 7};

LEDGroup ledGroup1(group1, 2);
LEDGroup ledGroup2(group2, 2);
LEDGroup ledGroup3(group3, 2);
LEDGroup ledGroup4(group4, 2);

void setup() {
    FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.clear();

    // Set initial configuration for groups
    ledGroup1.setColor(CRGB::Red);
    ledGroup2.setColor(CRGB::Green);
    ledGroup3.setColor(CRGB::Blue);
    ledGroup4.setColor(CRGB::Yellow);

    ledGroup1.setBlink(true, 300);
    ledGroup2.setBlink(false);
    ledGroup3.setIntensity(128);
    ledGroup4.setIntensity(64);
}

void loop() {
    ledGroup1.update();
    ledGroup2.update();
    ledGroup3.update();
    ledGroup4.update();
}
