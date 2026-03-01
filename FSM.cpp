#include "FSM.hpp"

FSM::FSM(const FSM_Struct& fsmStruct, const std::vector<std::shared_ptr<Action>>& actions)
    : name(fsmStruct.name)
{
    if (fsmStruct.initAction.has_value()) {
        auto it = std::find_if(actions.begin(), actions.end(),
            [&fsmStruct](const std::shared_ptr<Action>& a) {return a->getName() == fsmStruct.initAction.value();});
        if (it != actions.end()) {
            initAction = *it;
            initAction->performAction();
        }
        else
            throw std::runtime_error("Action is not defined");
    }
    states.reserve(fsmStruct.states.size());
    for (const auto& stateStruct : fsmStruct.states) {
        states.emplace_back(stateStruct, actions);
        if (states.back().getName() == fsmStruct.currentStateName)
            currentState = &states.back();
    }
}

void FSM::processTrigger(std::string trigger) {
    std::string newStateName = currentState -> processTrigger(trigger);
    for (auto& state : states) {
        if (state.getName() == newStateName) {
            if (&state == currentState) //Brak zmiany stanu wiêc nic nie robimy
                break;
            else {
                currentState = &state;
                if (state.getOnEnterAction() != nullptr) {
                    state.getOnEnterAction()->performAction();
                }
            }
        }
    }
}