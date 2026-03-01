#include "Action.hpp"
#include <string>
#include <vector>
#include "RCRTypes.hpp"

Action::Action(ActionStruct actionStruct, std::function<void(const std::string&)> executor)
    : name(actionStruct.name), type(actionStruct.type), commands(actionStruct.commands), executor(executor)
{}

void Action::performAction() {
    for (auto& command : commands) {
        executor(command);
    }
}