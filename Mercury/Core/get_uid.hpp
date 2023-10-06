#pragma once

#include <fstream>
#include "helpers.hpp"


std::wstring GetUIDFromFile() {
    std::wstring uid;
    const int MAX_RETRIES = 10;
    int attempt = 0;

    // Get the temporary directory path and append the .uid filename
    std::string tmpDir = TmpPath();
    const std::wstring fullpath = std::wstring(tmpDir.begin(), tmpDir.end()) + L".uid";

    while (attempt < MAX_RETRIES) {
        std::wifstream infile(fullpath);

        if (infile.is_open()) {
            std::getline(infile, uid);
            infile.close();
            return uid;
        }
        else {
            //std::cerr << "Failed to open .uid file. Attempt: " << (attempt + 1) << std::endl;
            Sleep(500);  // Wait for 500 milliseconds
        }

        attempt++;
    }

    return L"";
}

