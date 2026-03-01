#include <string>
#include <optional>
#include "RCRTypes.hpp"
#include "StateTransition.hpp"

StateTransition::StateTransition(const StateTransitionStruct& stateTransitionStruct, std::vector<std::shared_ptr<Action>> actions) {
    trigger = stateTransitionStruct.trigger;
    targetState = stateTransitionStruct.targetState;
    if (stateTransitionStruct.action.has_value()) {
        auto it = std::find_if(actions.begin(), actions.end(),
            [&stateTransitionStruct](const std::shared_ptr<Action>& a) {return a->getName() == stateTransitionStruct.action.value();});
        if (it != actions.end()) {
            action = *it;
        }
        else
            throw std::runtime_error("Action is not defined");
    }
}

std::string StateTransition::performTransition() {
    if (action != nullptr) {
        action->performAction();
    }
    return targetState;
}