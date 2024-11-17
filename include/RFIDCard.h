#ifndef RFIDCARD_H
#define RFIDCARD_H

#include <string>

class RFIDCard {
public:
    RFIDCard(const std::string& id, const std::string& mqttMessage);

    void setID(const std::string& id);
    void setMQTTMessage(const std::string& mqttMessage);

    std::string getID() const;
    std::string getMQTTMessage() const;

private:
    std::string id;
    std::string mqttMessage;
};

#endif // RFIDCARD_H
