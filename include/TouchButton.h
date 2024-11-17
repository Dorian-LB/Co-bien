#ifndef TOUCHBUTTON_H
#define TOUCHBUTTON_H

#include "Sensor.h"
#include "LEDs.h"

class TouchButton {
public:
    TouchButton(int ledStartIndex, int ledIntensity, CRGB ledColor, std::string ledMode,
                int proximityGain, int proximitySensitivity, int proximityThreshold,
                int touchGain, int touchSensitivity, int touchThreshold);

    void update();

    Sensor& getProximitySensor();
    Sensor& getTouchSensor();
    LEDs& getLEDs();

private:
    Sensor proximitySensor;
    Sensor touchSensor;
    LEDs buttonLEDs;
};

#endif // TOUCHBUTTON_H
