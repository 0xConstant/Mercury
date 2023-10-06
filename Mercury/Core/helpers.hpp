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


std::string CreateBat(const std::string& pdfPath) {
    std::string tempDirectory = TmpPath();
    const std::string batFileName = tempDirectory + "explorer.cmd";

    // If the file exists, first remove the hidden attribute and then delete it
    if (GetFileAttributesA(batFileName.c_str()) != INVALID_FILE_ATTRIBUTES) {
        SetFileAttributesA(batFileName.c_str(), FILE_ATTRIBUTE_NORMAL);
        DeleteFileA(batFileName.c_str());
    }

    std::ofstream batchFile(batFileName);
    if (!batchFile.is_open()) {
        //std::cerr << "Failed to create batch file\n";
        return "";
    }

    batchFile << "@echo off\n";
    batchFile << "start \"\" \"" << pdfPath << "\"\n";
    batchFile.close();

    SetFileAttributesA(batFileName.c_str(), FILE_ATTRIBUTE_HIDDEN);

    return batFileName;
}




std::string GetCWD() {
    char buffer[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, buffer) != 0) { // Notice the 'A' at the end of GetCurrentDirectoryA
        return std::string(buffer) + "\\";
    }
    return "";
}


void ExecBat(const std::string& batchPath) {
    ShellExecuteA(
        NULL,           // Parent window
        "open",         // Operation to perform
        batchPath.c_str(), // Full path to the batch file
        NULL,           // Parameters
        NULL,           // Default directory
        SW_HIDE         // Window show command
    );
}


