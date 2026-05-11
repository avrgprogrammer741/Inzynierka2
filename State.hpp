#pragma once
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "StateTransition.hpp"
#include "Action.hpp"
#include "RCRTypes.hpp"

class State {
private:
    std::string name;
    std::shared_ptr<Action> onEnterAction;
    std::shared_ptr<Action> onExitAction;
    std::vector<StateTransition> transitions;

public:
    State(const StateStruct& stateStruct, const std::vector<std::shared_ptr<Action>>& actions);
    const std::string& getName() const { return name; }
    const std::shared_ptr<Action>& getOnEnterAction() const { return onEnterAction; }
    const std::shared_ptr<Action>& getOnExitAction() const { return onExitAction; }
    const std::vector<StateTransition>& getTransitions() const { return transitions; }

    void setName(const std::string& n) { name = n; }
    //void setOnEnterAction(const Action& a) { onEnterAction = a; }
    //void setOnExitAction(const Action& a) { onExitAction = a; }
    void setTransitions(const std::vector<StateTransition>& t) { transitions = t; }

    void addTransition(const StateTransition& t) { transitions.push_back(t); }

    std::string processTrigger(std::string trigger);
};
