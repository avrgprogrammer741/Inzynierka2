#pragma once
#include <mqtt/async_client.h>
#include <memory>

class Component;

class Receiver : public virtual mqtt::callback {
    std::weak_ptr<Component> component;

public:
    void processTrigger(std::string& trigger);

    void message_arrived(mqtt::const_message_ptr msg) override;

    void setComponent(std::shared_ptr<Component> comp) {
        component = comp;
    }
};
