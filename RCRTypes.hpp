#pragma once
#include<iostream>
#include<variant>
#include<vector>
#include<map>
#include<optional>

struct ActionStruct {
    std::string name;
    std::string type;
    std::vector<std::string> commands;
};

struct TimerStruct {
    std::string index;
    std::string timeMicroSeconds;
};

struct TransportStruct {
    std::string name;
    std::string protocol;
    std::string ip;
    std::string port;
    std::string encoding;
};

struct PortStruct {
    std::string name;
    std::string transportName;
    std::string direction;
    std::string endpoint = "";
};

struct MessageStruct {
    std::string name;
    std::map<std::string, std::string> fieldAndType;
};

struct StateTransitionStruct {
    std::string trigger;
    std::string targetState;
    std::optional<std::string> action;
};

struct StateStruct {
    std::string name;
    std::optional<std::string> onEnterAction;
    std::optional<std::string> onExitAction;
    std::vector<StateTransitionStruct> transitionStructs;
};

struct FSM_Struct {
    std::string name;
    std::optional<std::string> initAction;
    std::string currentStateName;
    std::vector<StateStruct> states;
};

struct ComponentStruct {
    std::string name;
    int versionNumber;
    std::optional<std::string> onInitAction;
    std::optional<std::string> parameters;
    std::vector<std::string> actions;
    std::vector<std::string> messages;
    std::vector<std::string> transports;
    std::vector<std::string> ports;
    std::vector<std::string> fsms;
    std::vector<std::string> timers;
};