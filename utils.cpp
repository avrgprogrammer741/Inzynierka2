#pragma once
#include "utils.hpp"
#include <string>
#include <stdexcept>
#include <iostream>
#include <vector>
#include <map>


std::string sliceBetweenPosAndSemicolonAndJumpIterator(const std::string& buffer, int& i) {
	size_t endPos = buffer.find(';', i);
	if (endPos == std::string::npos) //sprawdzamy czy istnieje œrednik
		throw std::runtime_error("';' missing");

	// sprawdzamy, czy istnieje klamra przed œrednikiem lub po œredniku
	size_t nextOpeningBracket = buffer.find('{', i);
	if (nextOpeningBracket != std::string::npos && nextOpeningBracket < endPos) {
		std::cout << "Blad w: " << i << "\n";
		std::cout << "Znak: " << buffer[i] << "\n";
		throw std::runtime_error("Expected ';' before '{'");
	}

	size_t nextClosingBracket = buffer.find('}', i);
	if (nextClosingBracket != std::string::npos && nextClosingBracket < endPos) {
		std::cout << "Blad w: " << i << "\n";
		std::cout << "Znak: " << buffer[i] << "\n";
		throw std::runtime_error("Expected ';' before '}'");
	}



	std::string data = buffer.substr(i, endPos - i);
	i = static_cast<int>(endPos) - 1;// przeskakujemy do koñca wycinka, przed œrednik
	return data;
}

std::string trim(const std::string& s) {
	size_t start = s.find_first_not_of(" \t\r\n");
	size_t end = s.find_last_not_of(" \t\r\n");
	if (start == std::string::npos) return "";   // same spacje
	return s.substr(start, end - start + 1);
}

std::map<std::string, std::string> separateKeyAndValue(const std::string& field) {
	std::map<std::string, std::string> result;

	size_t pos = field.find(':');
	if (pos == std::string::npos) {
		// Brak dwukropka — zwracamy pust¹ mapê albo klucz = ca³y string, wartoœæ = ""
		return result;
	}

	std::string key = trim(field.substr(0, pos));
	std::string value = trim(field.substr(pos + 1));

	result[key] = value;
	return result;
}

std::string extractMessageNameFromTopic(const std::string& topic) {
	auto pos = topic.rfind('/');
	if (pos == std::string::npos) {
		return topic; // jeœli nie ma '/', ca³e topic jest nazw¹ wiadomoœci
	}
	return topic.substr(pos + 1);
}