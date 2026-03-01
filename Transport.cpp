#include <string>
#include "Transport.hpp"
#include <mosquitto.h>
#include <mqtt/async_client.h>
#include <sstream>

Transport::Transport(const TransportStruct& transportStruct) :client(
    "tcp://" + transportStruct.ip + ":" + transportStruct.port,
    transportStruct.name
),
name(transportStruct.name),
protocol(transportStruct.protocol),
ip(transportStruct.ip),
port(std::stoi(transportStruct.port)),
encoding(transportStruct.encoding)
{
    connect();
}
void Transport::connect() {
    if (protocol != "mqtt") {
        throw std::runtime_error("Transport is not MQTT!");
    }

    mqtt::connect_options connOpts;
    connOpts.set_clean_session(true);
    client.connect(connOpts)->wait();
    connected = true;
}
void Transport::publishAMessage(const std::string& message, std::string& endpoint) {
    if (protocol != "mqtt") {
        throw std::runtime_error("Transport is not MQTT!");
    }

    try {
        // --- Publikacja ---
        mqtt::message_ptr pubmsg = mqtt::make_message(endpoint + "/" + message, message); //Na razie wiadomosc to jedynie nazwa wiadomosci
        pubmsg->set_qos(1);

        client.publish(pubmsg)->wait();
        std::cout << "Published message: " << endpoint + "/" + message << "\n";
    }
    catch (const mqtt::exception& exc) {
        std::cerr << "MQTT ERROR: " << exc.what() << std::endl;
    }
}
void Transport::subscribeMessage(const std::string& messageName, const std::string& endpoint) {
    client.subscribe(endpoint + "/" + messageName, 1)->wait();
}
void Transport::disconnect() {
    client.disconnect()->wait();
}
void Transport::setReceiver(std::shared_ptr<Receiver> rec) {
    receiver = rec;
    client.set_callback(*receiver);
}