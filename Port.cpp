#include <string>
#include <optional>
#include <memory>
#include "utils.hpp"
#include "Port.hpp"

Port::Port(PortStruct& portStruct, std::shared_ptr<Transport> transport_ptr)
    : name(portStruct.name),
    transport(std::move(transport_ptr)),
    direction(portStruct.direction),
    endpoint(portStruct.endpoint)
{
    subscribeMessage("message1");
    subscribeMessage("message2");
}