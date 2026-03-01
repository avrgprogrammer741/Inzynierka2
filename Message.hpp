#pragma once
#include <string>
#include <vector>
#include <variant>
#include <sstream>
#include <map>
#include "RCRTypes.hpp"


class Message {
private:
    std::string name;
    std::map<std::string, std::string> fieldsAndTypes;

public:
    Message(MessageStruct messageStruct);
    // Gettery
    const std::string& getName() const { return name; }
    const std::map<std::string, std::string>& getFields() const { return fieldsAndTypes; }

    // Settery
    void setName(const std::string& n) { name = n; }

    // Dodanie pola
    void addField(const std::string& field, const std::string& type) { fieldsAndTypes[field] = type; }
};
