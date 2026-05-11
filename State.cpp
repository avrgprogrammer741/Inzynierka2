#include <string>
#include <vector>
#include <optional>
#include "RCRTypes.hpp"
#include "State.hpp"

State::State(const StateStruct& stateStruct, const std::vector<std::shared_ptr<Action>>& actions) {
    name = stateStruct.name;
    if (stateStruct.onEnterAction.has_value()) {
        auto it = std::find_if(actions.begin(), actions.end(),
            [&stateStruct](const std::shared_ptr<Action>& a) {return a->getName() == stateStruct.onEnterAction.value();});
        if (it != actions.end()) {
            onEnterAction = *it;
        }
        else
            throw std::runtime_error("Action is not defined");
    }
    if (stateStruct.onExitAction.has_value()) {
        auto it = std::find_if(actions.begin(), actions.end(),
            [&stateStruct](const std::shared_ptr<Action>& a) {return a->getName() == stateStruct.onExitAction.value();});
        if (it != actions.end()) {
            onExitAction = *it;
        }
        else
            throw std::runtime_error("Action is not defined");
    }
    for (auto& transitionStruct : stateStruct.transitionStructs) {
        transitions.emplace_back(transitionStruct, actions);
    }
}

std::string State::processTrigger(std::string trigger) { //Zwraca swoja nazwe jesli nie znajdzie zadnej tranzycji
    std::string stateName = name;
    for (auto& transition : transitions) {
        if (transition.trigger == trigger) {
            std::cout << "Przejscie ze stanu " << name << "\n";
            std::cout << "Do stanu " << transition.targetState << "\n";
            if (onExitAction != nullptr) {
                onExitAction->performAction();
            }
            stateName = transition.performTransition();
            break;
        }
    }
    return stateName;
}