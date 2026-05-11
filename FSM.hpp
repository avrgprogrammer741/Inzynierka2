#pragma once
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include "State.hpp"
#include "Action.hpp"
#include "RCRTypes.hpp"

class FSM {
private:
    std::string name;
    std::shared_ptr<Action> initAction;
    State* currentState = nullptr;
    std::vector<State> states;

public:
    FSM(const FSM_Struct& fsmStruct, const std::vector<std::shared_ptr<Action>>& actions);

    const std::string& getName() const { return name; }
    //const std::optional<Action>& getInitAction() const { return initAction; }
    const std::vector<State>& getStates() const { return states; }

    void setName(const std::string& n) { name = n; }
    //void setInitAction(const Action& a) { initAction = a; }
    void setStates(const std::vector<State>& s) { states = s; }
    void addState(const State& s) { states.push_back(s); }

    void processTrigger(std::string trigger);

};
