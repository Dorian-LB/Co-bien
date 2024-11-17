#ifndef SENSOR_H
#define SENSOR_H

class Sensor {
public:
    Sensor(int gain, int sensitivity, int threshold);

    void setGain(int gain);
    void setSensitivity(int sensitivity);
    void setThreshold(int threshold);

    int getGain() const;
    int getSensitivity() const;
    int getThreshold() const;

private:
    int gain;
    int sensitivity;
    int threshold;
};

#endif // SENSOR_H
