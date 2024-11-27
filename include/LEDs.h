#ifndef LEDS_H
#define LEDS_H

#include <FastLED.h>

extern CRGB leds[]; // Declare the LED array globally

enum LEDMode {
    ON,
    OFF,
    BLINK,
    FADING_BLINK
};

class LEDGroup {
private:
    int *indices;          // Indices of LEDs in the group
    int size;              // Number of LEDs in the group
    CRGB color;            // Current color
    uint8_t intensity;     // Maximum brightness
    bool state;            // ON/OFF state
    LEDMode mode;          // Current mode
    unsigned long lastUpdate;  // Time of the last update
    unsigned int interval; // Update interval in milliseconds
    uint8_t fadeLevel;     // Current fade level for fading blink
    int fadeDirection;     // Direction of fading (1 for up, -1 for down)

public:
    LEDGroup(int *ledIndices, int ledCount, unsigned int interval = 500);
    void configure(CRGB newColor,uint8_t newIntensity,LEDMode newMode, unsigned int newInterval = 500);
    // void setColor(CRGB newColor);
    // void setIntensity(uint8_t newIntensity);
    // void setMode(LEDMode newMode, unsigned int newInterval = 500);
    void update();
};

#endif