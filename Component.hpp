#pragma once
#include <string>
#include <vector>
#include <map>
#include <memory>
#include "Port.hpp"
#include "Message.hpp"
#include "FSM.hpp"
#include "Timer.hpp"
#include "Interpreter.hpp"
#include "Action.hpp"
#include "Transport.hpp"
#include "RCRTypes.hpp"

class Component : public std::enable_shared_from_this<Component> {
private:
    std::string name;
    int versionNumber;
    std::shared_ptr<Action> initAction;
    std::vector<Message> messages;
    std::vector<Port> ports;
    std::vector<std::shared_ptr<Transport>> transports;
    std::vector<FSM> fsms;
    std::vector<Timer> timers;
    std::vector<std::shared_ptr<Action>> actions;
    std::unique_ptr<state_t> forthState;

public:
    Component(std::string name, int version, std::optional<std::string> onInitAction, std::vector<ActionStruct> actStructs,
        std::vector<MessageStruct> messStructs,std::vector<TransportStruct> transStructs, std::vector<PortStruct> portStructs, std::vector<FSM_Struct> fsmStructs,
        std::vector<TimerStruct> timerStructs);
    void postInit();
    const std::string& getName() const { return name; }
    int getVersionNumber() const { return versionNumber; }
    const std::vector<Message>& getMessages() const { return messages; }
    const std::vector<Timer>& getTimers() const { return timers; }
    std::shared_ptr<Transport> getTransportByName(const std::string& name);
    Port& getPortByName(const std::string& name);

    void setName(const std::string& n) { name = n; }
    void setVersionNumber(int v) { versionNumber = v; }
    void setMessages(const std::vector<Message>& m) { messages = m; }
    void setTimers(const std::vector<Timer>& t) { timers = t; }

    void addMessage(const Message& m) { messages.push_back(m); }
    //void addFSM(const std::shared_ptr<FSM> f) { fsms.push_back(f); }
    void addTimer(const Timer& t) { timers.push_back(t); }

    void processTrigger(std::string& trigger);
    void interpretCommand(const std::string& command);
};