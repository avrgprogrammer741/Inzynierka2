#pragma once
#include <string>
#include <optional>
#include <memory>
#include <vector>
#include "Action.hpp"
#include "RCRTypes.hpp"

struct StateTransition {
    std::string trigger;
    std::string targetState;
    std::shared_ptr<Action> action;

public:
    StateTransition(const StateTransitionStruct& stateTransitionStruct, std::vector<std::shared_ptr<Action>> actions);
    std::string performTransition();
    std::string getTrigger() {
        return trigger;
    }
};
