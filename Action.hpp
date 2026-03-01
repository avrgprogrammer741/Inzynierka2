#pragma once
#include <string>
#include <vector>
#include <functional>
#include "RCRTypes.hpp"

class Action {
private:
    std::string name;
    std::string type;
    std::vector<std::string> commands;

    std::function<void(const std::string&)> executor;

public:
    Action(ActionStruct actionStruct, std::function<void(const std::string&)> executor);

    const std::string& getName() const { return name; }
    std::string getType() const { return type; }
    const std::vector<std::string>& getCommands() const { return commands; }

    void setName(const std::string& n) { name = n; }
    void setType(std::string t) { type = t; }
    void setCommands(const std::vector<std::string>& cmds) { commands = cmds; }
    void addCommand(const std::string& cmd) { commands.push_back(cmd); }

    void setExecutor(std::function<void(const std::string&)> exec) {
        executor = exec;
    }

    void performAction();
};