#pragma once
#include <string>
#include <memory>
#include "Message.hpp"
#include "Receiver.hpp"
#include "RCRTypes.hpp"

class Transport {
private:
    std::string name;
    std::string protocol;
    std::string ip;
    int port;
    std::string encoding;
    mqtt::async_client client;
    std::shared_ptr<Receiver> receiver;
    bool connected = false;

public:
    Transport(const TransportStruct& transportStruct);
    const std::string& getName() const { return name; }
    std::string getProtocol() const { return protocol; }
    const std::string& getIP() const { return ip; }
    int getPort() const { return port; }
    std::string getEncoding() const { return encoding; }

    void setName(const std::string& n) { name = n; }
    void setProtocol(std::string p) { protocol = p; }
    void setIP(const std::string& address) { ip = address; }
    void setPort(int p) { port = p; }
    void setEncoding(std::string e) { encoding = e; }
    void setReceiver(std::shared_ptr<Receiver> receiver);
    void publishAMessage(const std::string& message, const std::string& prefix);
    void subscribeMessage(const std::string& messageName, const std::string& prefix);
    void connect();
    void disconnect();

};
