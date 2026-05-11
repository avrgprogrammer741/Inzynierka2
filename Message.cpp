#include <string>
#include <vector>
#include <variant>
#include <sstream>
#include <map>
#include "RCRTypes.hpp"
#include "Message.hpp"

Message::Message(MessageStruct messageStruct) {
    name = messageStruct.name;
    fieldsAndTypes = messageStruct.fieldAndType;
}