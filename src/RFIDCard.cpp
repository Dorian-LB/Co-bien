#include "RFIDCard.h"

RFIDCard::RFIDCard(const std::string& id, const std::string& mqttMessage)
    : id(id), mqttMessage(mqttMessage) {}

void RFIDCard::setID(const std::string& newID) {
    id = newID;
}

void RFIDCard::setMQTTMessage(const std::string& newMQTTMessage) {
    mqttMessage = newMQTTMessage;
}

std::string RFIDCard::getID() const {
    return id;
}

std::string RFIDCard::getMQTTMessage() const {
    return mqttMessage;
}