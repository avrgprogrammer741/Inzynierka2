#pragma once
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <map>


std::string sliceBetweenPosAndSemicolonAndJumpIterator(const std::string& buffer, int& i);
std::string trim(const std::string& s);
std::map<std::string, std::string> separateKeyAndValue(const std::string& field);
std::string extractMessageNameFromTopic(const std::string& topic);