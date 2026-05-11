#define _CRT_SECURE_NO_WARNINGS
#include "Component.hpp"


void Component::processTrigger(std::string& trigger) {
    for (auto& fsm : fsms) {
        fsm.processTrigger(trigger);
    }
}

std::shared_ptr<Transport> Component::getTransportByName(const std::string& name) {
    auto it = std::find_if(transports.begin(), transports.end(),
        [name](const std::shared_ptr<Transport>& t) {return t->getName() == name;});
    if (it != transports.end())
        return *it;
    else
        throw std::runtime_error("Transport with name " + name + " does not exist");
}

Port& Component::getPortByName(const std::string& name) {
    auto it = std::find_if(ports.begin(), ports.end(),
        [name](Port& p) {return p.getName() == name;});
    if (it != ports.end())
        return *it;
    else
        throw std::runtime_error("Port with name " + name + " does not exist");
}

void Component::postInit()
{
    auto receiver = std::make_shared<Receiver>();
    receiver->setComponent(shared_from_this());

    for (auto& trans : transports) {
        trans->setReceiver(receiver);
    }
}

Component::Component(std::string name, int version, std::optional<std::string> onInitAction, std::vector<ActionStruct> actionStructs,
    std::vector<MessageStruct> messageStructs, std::vector<TransportStruct> transStructs, std::vector<PortStruct> portStructs, std::vector<FSM_Struct> fsmStructs,
    std::vector<TimerStruct> timerStructs)
{
    forthState = std::unique_ptr<state_t>(create_state());
    create_dictionary(forthState.get());
    for (auto& messStruct : messageStructs) {
        messages.emplace_back(messStruct);
    }
    for (auto& timerStruct : timerStructs) {
        timers.emplace_back(timerStruct);
    }
    for (auto& transStruct : transStructs) {
        transports.push_back(
            std::make_shared<Transport>(
                transStruct
            )
        );
    }
    for (auto& portStruct : portStructs) {
        ports.emplace_back(portStruct, getTransportByName(portStruct.transportName), name);
    }
    for (auto& actStruct : actionStructs) {
        actions.push_back(std::make_shared<Action>(
            actStruct,
            [this](const std::string& cmd) { interpretCommand(cmd); }
        ));
    }
    if (onInitAction.has_value()) {
        auto it = std::find_if(actions.begin(), actions.end(),
            [&onInitAction](const std::shared_ptr<Action>& a) {return a->getName() == onInitAction.value();});
        if (it != actions.end()) {
            initAction = *it;
            initAction->performAction();
        }
        else
            throw std::runtime_error("Action is not defined");
    }


    for (auto& fsmStruct : fsmStructs) {
        fsms.emplace_back(fsmStruct, actions);
    }

}

void Component::interpretCommand(const std::string& command) {
        FILE* f = fmemopen(
            (void*)command.c_str(),
            command.size(),
            "r"
        );

    if (!f) {
        throw std::runtime_error("Error creating memory stream");
    }

    forthState->input = f;
    forthState->output = stdout;

    interpret(forthState.get());

    fclose(f);
}
