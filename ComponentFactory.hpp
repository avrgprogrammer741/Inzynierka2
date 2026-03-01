#pragma once
#include <string>
#include <vector>
#include "RCRTypes.hpp"
#include "Component.hpp"

class ComponentFactory {
public:
	std::string loadFile(const std::string& filename);
	std::vector<std::shared_ptr<Component>> readRCRFile(const std::string & filename);
};
