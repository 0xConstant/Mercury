#pragma once

#include <iostream>
#include <fstream>
#include <nlohmann/json.hpp>


void SaveJSON(const std::string& filePath, const nlohmann::json& jsonData) {
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << jsonData.dump(4); // Indented with 4 spaces for readability
        file.close();
        //std::cout << "JSON saved to " << filePath << std::endl;
    }
    else {
        //std::cerr << "Failed to open file " << filePath << " for writing" << std::endl;
    }
}
