#ifndef LEDS_H
#define LEDS_H

#include <FastLED.h>
#include <string>

class LEDs {
public:
    LEDs(int numLeds, int startIndex, int intensity, CRGB color, std::string mode);
    
    void setColor(CRGB color);
    void setIntensity(int intensity);
    void setMode(const std::string& mode);
    void update();

private:
    int numLeds;
    int startIndex;
    int intensity;
    CRGB color;
    std::string mode; // "blink" or "steady"
    unsigned long lastUpdate;
    bool isOn;

    void blink();
    void steady();
};

#endif // LEDS_H
