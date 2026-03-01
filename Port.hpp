#pragma once
#include <string>
#include <optional>
#include <memory>
#include "Transport.hpp"
#include "RCRTypes.hpp"

class Port {
private:
    std::string name;
    std::shared_ptr<Transport> transport;
    std::string direction;
    std::string endpoint;

public:
    Port(PortStruct& portStruct, std::shared_ptr<Transport> transport_ptr);
    void publishAMessage(const std::string& message) {
        transport->publishAMessage(message, endpoint);
    }
    void subscribeMessage(const std::string& messageName) {
        transport->subscribeMessage(messageName, endpoint);
    }
    const std::string& getName() const { return name; }
    const std::shared_ptr<Transport> getTransport() const { return transport; }
    std::string getDirection() const { return direction; }
    std::string getReceiver() const { return endpoint; }
    std::string getProtocol() const {
        return transport->getProtocol();
    }
    void setName(const std::string& n) { name = n; }
    void setDirection(std::string& d) { direction = d; }
    void setReceiver(std::string e) { endpoint = e; }
};
