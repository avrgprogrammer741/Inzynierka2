#include "Receiver.hpp"
#include "Component.hpp"
#include "utils.hpp"

void Receiver::processTrigger(std::string& trigger) {
    if (auto comp = component.lock()) {
        comp->processTrigger(trigger);
    }
}

void Receiver::message_arrived(mqtt::const_message_ptr msg) {
    std::cout << "[Receiver] message_arrived CALLED\n";
    std::string trigger = extractMessageNameFromTopic(msg->get_topic());
    processTrigger(trigger);
}