#pragma once

#include <fstream>


std::wstring GetUIDFromFile() {
    std::wstring uid;
    const int MAX_RETRIES = 10;
    int attempt = 0;

    while (attempt < MAX_RETRIES) {
        std::wifstream infile(L".uid");

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

