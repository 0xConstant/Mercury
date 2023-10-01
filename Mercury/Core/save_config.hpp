#pragma once

#include <fstream>
#include "json.hpp"


void add_batch(const std::string& filename, const std::string configFile) {
    nlohmann::json batches;
    std::ifstream input(configFile);

    // Read existing batches if the file exists
    if (input.good()) {
        input >> batches;
        input.close();
    }

    // Append new batch to the list
    batches.push_back({
        {"id", static_cast<int>(batches.size()) + 1},
        {"filename", filename}
        });

    std::ofstream output(configFile);
    output << batches.dump(4);  // 4 spaces for indentation
    output.close();
}
