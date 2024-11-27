#include "LEDs.h"

LEDGroup::LEDGroup(int *ledIndices, int ledCount, unsigned int interval)
    : indices(ledIndices), size(ledCount), color(CRGB::Black), intensity(255),
      state(true), mode(OFF), lastUpdate(0), interval(interval),
      fadeLevel(0), fadeDirection(1) {}

void LEDGroup::configure(CRGB newColor,uint8_t newIntensity,LEDMode newMode, unsigned int newInterval) {
    //Color
    color = newColor;
    //Intensity
    intensity = newIntensity;
    //Mode
    mode = newMode;
    interval = newInterval;
    fadeLevel = 0;          // Reset fade level for fading mode
    fadeDirection = 1;      // Reset fade direction
    lastUpdate = millis();  // Reset timer

}

void LEDGroup::update() {
    unsigned long currentTime = millis();
    if (currentTime - lastUpdate >= interval) {
        lastUpdate = currentTime;

        if (mode == FADING_BLINK) {
            // Update fade level
            fadeLevel += fadeDirection * 15; // Adjust fading step 
            if (fadeLevel >= 255 || fadeLevel <= 0) {
                fadeDirection *= -1; // Reverse fading direction
            }

            // Set LEDs to the current fade level
            for (int i = 0; i < size; i++) {
                leds[indices[i]] = color;
                leds[indices[i]].subtractFromRGB(255-intensity);
                leds[indices[i]].fadeToBlackBy(255 - fadeLevel);
            }

        } else if (mode == BLINK) {
            // Toggle ON/OFF state 
            state = !state;
            // Apply ON/OFF state to LEDs
            for (int i = 0; i < size; i++) {
                leds[indices[i]] = state ? color : CRGB::Black;
                leds[indices[i]].subtractFromRGB(255-intensity);

            }
        } else if (mode == ON ) {
            // Apply ON state to LEDs
            for (int i = 0; i < size; i++) {
                leds[indices[i]] =  color;
                leds[indices[i]].subtractFromRGB(255-intensity);

            }
        } else if (mode == OFF ) {
            // Apply OFF state to LEDs
            for (int i = 0; i < size; i++) {
                leds[indices[i]] =  CRGB::Black;
            }
        }
        FastLED.show();
    }
}
