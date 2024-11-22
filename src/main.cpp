#include <FastLED.h>
#include "LEDs.h"

#define NUM_LEDS 8
#define DATA_PIN 16

CRGB leds[NUM_LEDS];

int group1[] = {0, 1};
int group2[] = {2, 3};
int group3[] = {4, 5};
int group4[] = {6, 7};

LEDGroup ledGroup1(group1, 2);
LEDGroup ledGroup2(group2, 2);
LEDGroup ledGroup3(group3, 2);
LEDGroup ledGroup4(group4, 2);

void setup() {
    FastLED.addLeds<SK6812, DATA_PIN, GRB>(leds, NUM_LEDS);
    FastLED.clear();

    ledGroup1.setColor(CRGB::Red);
    ledGroup2.setColor(CRGB::Green);
    ledGroup3.setColor(CRGB::Blue);
    ledGroup4.setColor(CRGB::Yellow);

    ledGroup1.setMode(FADING_BLINK, 50); // Fast fading blink
    ledGroup2.setMode(BLINK);           // BLINK ON/OFF mode
    ledGroup3.setMode(ON); // ONN
    ledGroup4.setMode(OFF);      // OFF
}

void loop() {
    ledGroup1.update();
    ledGroup2.update();
    ledGroup3.update();
    ledGroup4.update();
}