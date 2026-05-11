#include <string>
#include <optional>
#include <memory>
#include "utils.hpp"
#include "Port.hpp"

Port::Port(PortStruct& portStruct, std::shared_ptr<Transport> transport_ptr, const std::string& componentName)
    : name(portStruct.name),
    transport(std::move(transport_ptr)),
    direction(portStruct.direction),
    endpoint(portStruct.endpoint),
    componentName(componentName)
{
    if(direction != "out")
        subscribeMessage("#");
}