// Inzynierka2.cpp : Ten plik zawiera funkcję „main”. W nim rozpoczyna się i kończy wykonywanie programu.
//

#include <iostream>
#include <mosquitto.h>
#include <mqtt/async_client.h>
#include "ComponentFactory.hpp"
#include "utils.hpp"
#include "Windows.h"

int main()
{
    std::string filename = "C:\\Users\\user\\downloads\\test2 1.rcr";
    ComponentFactory compFactory;
    std::vector<std::shared_ptr<Component>> components = compFactory.readRCRFile(filename);
    Port& port1 = components[0]->getPortByName("port1");
    Port& port1_second = components[1]->getPortByName("port1");
    port1.publishAMessage("message1");
    Sleep(1000);
    std::cout << "Message sent\n";
    std::cout << "----------------------------------------------------------------------\n";
    port1.publishAMessage("message1");
    Sleep(1000);
    std::cout << "Message sent\n";
    std::cout << "----------------------------------------------------------------------\n";
    port1_second.publishAMessage("message1");
    Sleep(1000);
    std::cout << "Message sent\n";
    std::cout << "----------------------------------------------------------------------\n";
    port1.publishAMessage("message1");
    Sleep(1000);
    std::cout << "Message sent\n";
    std::cout << "----------------------------------------------------------------------\n";
    port1_second.publishAMessage("message1");
    Sleep(1000);
    std::cout << "Message sent\n";
    std::cout << "----------------------------------------------------------------------\n";
    port1.publishAMessage("message2");
    Sleep(1000);
    std::cout << "Message sent\n";
    std::cout << "----------------------------------------------------------------------\n";
    auto& t = port1.getTransport();
    t->disconnect();
    Sleep(500);
    while (true) {
        Sleep(1000);
    }
}

// Uruchomienie programu: Ctrl + F5 lub menu Debugowanie > Uruchom bez debugowania
// Debugowanie programu: F5 lub menu Debugowanie > Rozpocznij debugowanie

// Porady dotyczące rozpoczynania pracy:
//   1. Użyj okna Eksploratora rozwiązań, aby dodać pliki i zarządzać nimi
//   2. Użyj okna programu Team Explorer, aby nawiązać połączenie z kontrolą źródła
//   3. Użyj okna Dane wyjściowe, aby sprawdzić dane wyjściowe kompilacji i inne komunikaty
//   4. Użyj okna Lista błędów, aby zobaczyć błędy
//   5. Wybierz pozycję Projekt > Dodaj nowy element, aby utworzyć nowe pliki kodu, lub wybierz pozycję Projekt > Dodaj istniejący element, aby dodać istniejące pliku kodu do projektu
//   6. Aby w przyszłości ponownie otworzyć ten projekt, przejdź do pozycji Plik > Otwórz > Projekt i wybierz plik sln
