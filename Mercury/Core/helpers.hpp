#pragma once
#include <filesystem>
#include <Windows.h>
#include <fstream>


std::string TmpPath() {
    char buffer[MAX_PATH];
    DWORD length = GetTempPathA(MAX_PATH, buffer);
    if (length == 0 || length >= MAX_PATH) {
        //std::cerr << "Failed to get temp path\n";
        return "";
    }
    return std::string(buffer);
}


std::string GetCWD() {
    char buffer[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, buffer) != 0) { // Notice the 'A' at the end of GetCurrentDirectoryA
        return std::string(buffer) + "\\";
    }
    return "";
}



