#include <iostream>
#include <mosquitto.h>
#include <mqtt/async_client.h>
#include "ComponentFactory.hpp"
#include "utils.hpp"

#ifdef _WIN32
#include <Windows.h>
#else
#include <thread>
#include <chrono>
#endif

int main(int argc, char* argv[])
{
    if (argc < 2) {
        std::cerr << "Usage: Inzynierka2 <file.rcr>\n";
        return 1;
    }

        std::string filename = argv[1];

    ComponentFactory compFactory;
    std::vector<std::shared_ptr<Component>> components =
        compFactory.readRCRFile(filename);

    Port& port1 = components[0]->getPortByName("port1");
    Port& port1_second = components[1]->getPortByName("port1");

    port1.publishAMessage("message1");

#ifdef _WIN32
        Sleep(1000);
#else
        std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

        std::cout << "Message sent\n";
    std::cout << "----------------------------------------------------------------------\n";

    port1.publishAMessage("message1");

#ifdef _WIN32
        Sleep(1000);
#else
        std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

        std::cout << "Message sent\n";
    std::cout << "----------------------------------------------------------------------\n";

    port1_second.publishAMessage("message1");

#ifdef _WIN32
        Sleep(1000);
#else
        std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

        std::cout << "Message sent\n";
    std::cout << "----------------------------------------------------------------------\n";

    port1.publishAMessage("message1");

#ifdef _WIN32
        Sleep(1000);
#else
        std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

        std::cout << "Message sent\n";
    std::cout << "----------------------------------------------------------------------\n";

    port1_second.publishAMessage("message1");

#ifdef _WIN32
        Sleep(1000);
#else
        std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

        std::cout << "Message sent\n";
    std::cout << "----------------------------------------------------------------------\n";

    port1.publishAMessage("message2");

#ifdef _WIN32
        Sleep(1000);
#else
        std::this_thread::sleep_for(std::chrono::seconds(1));
#endif

        std::cout << "Message sent\n";
    std::cout << "----------------------------------------------------------------------\n";

    auto& t = port1.getTransport();
    t->disconnect();

#ifdef _WIN32
        Sleep(500);
#else
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
#endif

        while (true) {

#ifdef _WIN32
                Sleep(1000);
#else
                std::this_thread::sleep_for(std::chrono::seconds(1));
#endif
        }
}
