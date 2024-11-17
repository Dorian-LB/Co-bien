// #include "LEDs.h"

// LEDs::LEDs(int numLeds, int startIndex, int intensity, CRGB color, std::string mode)
//     : numLeds(numLeds), startIndex(startIndex), intensity(intensity), color(color), mode(mode), lastUpdate(0), isOn(true) {}

// void LEDs::setColor(CRGB newColor) {
//     color = newColor;
// }

// void LEDs::setIntensity(int newIntensity) {
//     intensity = newIntensity;
// }

// void LEDs::setMode(const std::string& newMode) {
//     mode = newMode;
// }

// void LEDs::update() {
//     if (mode == "blink") {
//         blink();
//     } else if (mode == "steady") {
//         steady();
//     }
// }

// void LEDs::blink() {
//     unsigned long currentTime = millis();
//     if (currentTime - lastUpdate >= 500) { // Toggle every 500 ms
//         isOn = !isOn;
//         lastUpdate = currentTime;
//     }

//     for (int i = startIndex; i < startIndex + numLeds; i++) {
//         leds[i] = isOn ? color : CRGB::Black;
//     }
//     FastLED.show();
// }

// void LEDs::steady() {
//     for (int i = startIndex; i < startIndex + numLeds; i++) {
//         leds[i] = color.nscale8(intensity);
//     }
//     FastLED.show();
// }
