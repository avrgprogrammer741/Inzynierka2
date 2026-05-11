#include "ComponentFactory.hpp"
#include "utils.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <map>
#include <vector>
#include "RCRTypes.hpp"
#include "Component.hpp"


std::string ComponentFactory::loadFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file)
        throw std::runtime_error("Cannot open file: " + filename);

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::vector<std::shared_ptr<Component>> ComponentFactory::readRCRFile(const std::string & filename) {
    std::vector<std::shared_ptr<Component>> components;    std::string buffer = loadFile(filename);
    int blockCount = 0, globalSemicolonCount = 0;
    char versionNumber;
    std::string name;
    std::optional<std::string> onInitAction;
    std::string data;
    std::map<int, int> localSemicolonCounts = {};
    ActionStruct* currentActionStruct = nullptr;
    FSM_Struct* currentFsmStruct = nullptr;
    MessageStruct* currentMessageStruct = nullptr;
    PortStruct* currentPortStruct = nullptr;
    StateStruct* currentStateStruct = nullptr;
    StateTransitionStruct* currentStateTransitionStruct = nullptr;
    TimerStruct* currentTimerStruct = nullptr;
    TransportStruct* currentTransportStruct = nullptr;
    std::vector<ActionStruct> actionStructs;
    std::vector<FSM_Struct> fsmStructs; 
    std::vector<MessageStruct> messageStructs; 
    std::vector<PortStruct> portStructs; 
    std::vector<StateStruct> stateStructs;
    std::vector<StateTransitionStruct> stateTransitionStructs;
    std::vector<TimerStruct> timerStructs;
    std::vector<TransportStruct> transportStructs;

    auto resetParser = [&]() {
        blockCount = 0;
        globalSemicolonCount = 0;
        name = "";
        onInitAction = std::nullopt;
        versionNumber = ' ';
        localSemicolonCounts.clear();

        currentActionStruct = nullptr;
        currentFsmStruct = nullptr;
        currentMessageStruct = nullptr;
        currentPortStruct = nullptr;
        currentStateStruct = nullptr;
        currentStateTransitionStruct = nullptr;
        currentTimerStruct = nullptr;
        currentTransportStruct = nullptr;
        actionStructs.clear();
        fsmStructs.clear();
        messageStructs.clear();
        portStructs.clear();
        stateStructs.clear();
        stateTransitionStructs.clear();
        timerStructs.clear();
        transportStructs.clear();
        };


    for (int i = 0; i < buffer.size(); i++) {
        if (buffer[i] == '\n' || buffer[i] == ' ' || buffer[i] == '\r' || buffer[i] == '\t')
            continue;

        if (buffer[i] == ';') {
            if (blockCount == 0 || blockCount == 1) {
                globalSemicolonCount++;
                if (globalSemicolonCount == 11) {
                    if (blockCount == 0) {
                        auto comp = std::make_shared<Component>(
                            name, versionNumber, onInitAction, actionStructs,
                            messageStructs, transportStructs,
                            portStructs, fsmStructs, timerStructs
                        );
                        comp->postInit();
                        components.push_back(comp);
                        resetParser();
                    }
                    else
                        throw std::runtime_error("Character out of place");
                }
            }
            else if (localSemicolonCounts.count(blockCount) > 0)
                localSemicolonCounts[blockCount]++;
            else
                localSemicolonCounts[blockCount] = 1;

        }

        else if (buffer[i] == '{')
            blockCount++;

        else if (buffer[i] == '}') {
            localSemicolonCounts[blockCount] = 0;
            blockCount--;
        }

        else {

            switch (globalSemicolonCount) {

            case 0: // Nr wersji
                if (isdigit(buffer[i])) {
                    versionNumber = buffer[i];
                    std::cout << "Nr wersji: " << buffer[i] << "\n";
                }
                else
                    throw std::runtime_error("Version number is not a character");
                break;

            case 1: // Nazwa komponentu
                if (blockCount != 1) {
                    std::cout << "B³¹d w: " << i << "\n";
                    throw std::runtime_error("Character out of place");
                }
                data = sliceBetweenPosAndSemicolonAndJumpIterator(buffer, i);
                name = data;
                std::cout << "Nazwa komponentu: " << data << "\n";
                break;

            case 2: // Akcje podczas inicjalizacji
                if (blockCount != 1 && blockCount != 2) {
                    std::cout << "B³¹d w: " << i << "\n";
                    throw std::runtime_error("Character out of place");
                }
                data = sliceBetweenPosAndSemicolonAndJumpIterator(buffer, i);
                onInitAction = data;
                std::cout << "Akcja podczas inicjalizacji: " << data << "\n";
                break;

            case 3: // Parametry komponentu
                if (blockCount != 1 && blockCount != 2) {
                    std::cout << "B³¹d w: " << i << "\n";
                    throw std::runtime_error("Character out of place");
                }
                data = sliceBetweenPosAndSemicolonAndJumpIterator(buffer, i);
                //parameters = data;
                std::cout << "Parametr komponentu: " << data << "\n";
                break;

            case 4: // Definicja akcji
                if (blockCount != 2) {
                    std::cout << "B³¹d w: " << i << "\n";
                    throw std::runtime_error("Character out of place");
                }
                data = sliceBetweenPosAndSemicolonAndJumpIterator(buffer, i);
                switch (localSemicolonCounts[blockCount]) {
                case 0:
                    std::cout << "Nazwa definiowanej akcji: " << data << "\n";
                    actionStructs.emplace_back();
                    currentActionStruct = &actionStructs.back();
                    currentActionStruct->name = data;
                    break;
                case 1:
                    std::cout << "Typ definiowanej akcji: " << data << "\n";
                    currentActionStruct->type = data;
                    break;
                default:
                    std::cout << "Instrukcja definiowanej akcji: " << data << "\n";
                    currentActionStruct->commands.push_back(data);
                    break;
                }
                break;

            case 5: // Definicja wiadomoœci
                if (blockCount != 1 && blockCount != 2) {
                    std::cout << "B³¹d w: " << i << "\n";
                    throw std::runtime_error("Character out of place");
                }
                data = sliceBetweenPosAndSemicolonAndJumpIterator(buffer, i);
                switch (localSemicolonCounts[blockCount]) {
                case 0:
                    std::cout << "Nazwa wiadomoœci: " << data << "\n";
                    messageStructs.emplace_back();
                    currentMessageStruct = &messageStructs.back();
                    currentMessageStruct->name = data;
                    break;
                default:
                    std::map<std::string, std::string> field = separateKeyAndValue(data);
                    std::string name = field.begin()->first;
                    std::string type = field[name];
                    std::cout << "Nazwa pola wiadomoœci: " << name << "\n";
                    std::cout << "Typ pola wiadomoœci: " << type << "\n";
                    currentMessageStruct->fieldAndType[name] = type;
                    break;
                }
                break;


            case 6: // Timer
                if (blockCount != 2) {
                    std::cout << "B³¹d w: " << i << "\n";
                    throw std::runtime_error("Character out of place");
                }
                data = sliceBetweenPosAndSemicolonAndJumpIterator(buffer, i);
                switch (localSemicolonCounts[blockCount]) {
                case 0:
                    std::cout << "Indeks timera: " << data << "\n";
                    timerStructs.emplace_back();
                    currentTimerStruct = &timerStructs.back();
                    currentTimerStruct->index = data;
                    break;
                case 1:
                    std::cout << "Czas timera: " << data << "\n";
                    currentTimerStruct->timeMicroSeconds = data;
                    break;
                }
                break;

            case 7: // Definicja protoko³u transportowego
                if (blockCount != 2) {
                    std::cout << "B³¹d w: " << i << "\n";
                    throw std::runtime_error("Character out of place");
                }
                data = sliceBetweenPosAndSemicolonAndJumpIterator(buffer, i);
                switch (localSemicolonCounts[blockCount]) {
                case 0:
                    std::cout << "Nazwa transportu: " << data << "\n";
                    transportStructs.emplace_back();
                    currentTransportStruct = &transportStructs.back();
                    currentTransportStruct->name = data;
                    break;
                case 1:
                    if (data != "http" && data != "udp" && data != "mqtt")
                        throw std::runtime_error("Unknown protocol name");
                    std::cout << "Protokol: " << data << "\n";
                    currentTransportStruct->protocol = data;
                    break;
                case 2:
                    std::cout << "Adres: " << data << "\n";
                    currentTransportStruct->ip = data;
                    break;
                case 3:
                    std::cout << "Port: " << data << "\n";
                    currentTransportStruct->port = data;
                    break;
                case 4:
                    if (data != "text" && data != "json" && data != "binary")
                        throw std::runtime_error("Unknown format name");
                    std::cout << "Format transportu: " << data << "\n";
                    currentTransportStruct->encoding = data;
                    break;
                }
                break;

            case 8: // Definicja maszyn stanów
                if (blockCount != 2 && blockCount != 3 && blockCount != 4) {
                    std::cout << "B³¹d w: " << i << "\n";
                    throw std::runtime_error("Character out of place");
                }

                data = sliceBetweenPosAndSemicolonAndJumpIterator(buffer, i);
                switch (blockCount) {

                case 2: // Definicja FSM
                    switch (localSemicolonCounts[blockCount]) {
                    case 0:
                        std::cout << "Nazwa FSM: " << data << "\n";
                        fsmStructs.emplace_back();
                        currentFsmStruct = &fsmStructs.back();
                        currentFsmStruct->name = data;
                        break;
                    case 1: {
                        std::cout << "Akcja podczas inicjalizacji FSM: " << data << "\n";
                        auto it = std::find_if(actionStructs.begin(), actionStructs.end(),
                            [data](ActionStruct a) {return a.name == data;});
                        if (it != actionStructs.end())
                            currentFsmStruct->initAction = it->name;
                        else
                            throw std::runtime_error("Action is not defined");
                        break;
                    }
                    case 2: {
                        std::cout << "Stan pocz¹tkowy: " << data << "\n";
                        currentFsmStruct->currentStateName = data;
                        currentFsmStruct->states.emplace_back();
                        currentStateStruct = &currentFsmStruct->states.back();
                        currentStateStruct->name = data;
                        break;
                    }
                    }
                    break;

                case 3: // Definicja stanów
                    switch (localSemicolonCounts[blockCount]) {
                    case 0: {
                        std::cout << "Nazwa stanu: " << data << "\n";
                        auto it = std::find_if(stateStructs.begin(), stateStructs.end(),
                            [data](StateStruct s) {return s.name == data;});
                        if (it == stateStructs.end()) {
                            currentFsmStruct->states.emplace_back();
                            currentStateStruct = &currentFsmStruct->states.back();
                            currentStateStruct->name = data;
                        }
                        break;
                    }
                    case 1: {
                        std::cout << "Akcja podczas wejœcia do stanu: " << data << "\n";
                        auto it = std::find_if(actionStructs.begin(), actionStructs.end(),
                            [data](ActionStruct a) {return a.name == data;});
                        if (it != actionStructs.end()) {
                            currentStateStruct->onEnterAction = it->name;
                        }
                        else {
                            throw std::runtime_error("Action is not defined");
                        }
                        break;
                    }
                    case 2: {
                        std::cout << "Akcja podczas wyjœcia ze stanu: " << data << "\n";
                        auto it = std::find_if(actionStructs.begin(), actionStructs.end(),
                            [data](ActionStruct a) {return a.name == data;});
                        if (it != actionStructs.end())
                            currentStateStruct->onExitAction = it->name;
                        else
                            throw std::runtime_error("Action is not defined");
                        break;
                    }
                    }
                    break;

                case 4: // Przejœcia miêdzy stanami
                    switch (localSemicolonCounts[blockCount]) {
                    case 0:
                        std::cout << "Trigger przejscia: " << data << "\n";
                        currentStateStruct->transitionStructs.emplace_back();
                        currentStateTransitionStruct = &currentStateStruct->transitionStructs.back();
                        currentStateTransitionStruct->trigger = data;
                        break;
                    case 1:
                        std::cout << "Stan docelowy: " << data << "\n";
                        currentStateTransitionStruct->targetState = data;
                        break;
                    case 2: {
                        std::cout << "Akcja podczas wejœcia do stanu: " << data << "\n";
                        auto it = std::find_if(actionStructs.begin(), actionStructs.end(),
                            [data](ActionStruct a) {return a.name == data;});
                        if (it != actionStructs.end()) {
                            currentStateTransitionStruct->action = it->name;
                        }
                        else {
                            throw std::runtime_error("Action is not defined");
                        }
                        break;
                    }
                    }
                    break;
                }

                break;

            case 9: // Porty
                if (blockCount != 2) {
                    std::cout << "B³¹d w: " << i << "\n";
                    throw std::runtime_error("Character out of place");
                }

                data = sliceBetweenPosAndSemicolonAndJumpIterator(buffer, i);

                switch (localSemicolonCounts[blockCount]) {
                case 0:
                    std::cout << "Nazwa portu: " << data << "\n";
                    portStructs.emplace_back();
                    currentPortStruct = &portStructs.back();
                    currentPortStruct->name = data;
                    break;
                case 1: {
                    std::cout << "Nazwa transportu portu: " << data << "\n";
                    currentPortStruct->transportName = data;
                    break;
                }
                case 2:
                    std::cout << "Typ (in lub out lub in/out): " << data << "\n";
                    currentPortStruct->direction = data;
                    break;
                case 3:
                    std::cout << "Endpoint portu: " << data << "\n";
                    currentPortStruct->endpoint = data;
                    break;
                }
                break;
            }
        }
    }
    return components;
}

