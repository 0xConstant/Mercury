#pragma once

#include <windows.h>
#include <fstream>
#include <iostream>


void Cleanup() {
    // Define the config file name
    const std::string configFileName = "config.json";

    // Check if config.json exists and if so, read and delete the files listed
    if (std::filesystem::exists(configFileName)) {
        std::ifstream configFile(configFileName);
        nlohmann::json configData;
        configFile >> configData;

        // Explicitly close the configFile
        configFile.close();

        for (const auto& entry : configData) {
            std::string filename = entry["filename"];
            if (std::filesystem::exists(filename)) {
                std::filesystem::remove(filename);
            }
        }

        // Delete the config.json file itself
        std::filesystem::remove(configFileName);
    }

    // Check for zipped.zip and delete it
    if (std::filesystem::exists("zipped.zip")) {
        std::filesystem::remove("zipped.zip");
    }

    // Check if run_count exist and delete it
    if (std::filesystem::exists("run_count.txt")) {
        std::filesystem::remove("run_count.txt");
    }

    // Remove the registry entry
    LONG lResult = RegDeleteTree(HKEY_CURRENT_USER, L"Software\\AdminTools");
    if (lResult != ERROR_SUCCESS) {
        std::cerr << "Failed to delete the registry key." << std::endl;
    }

    // Delete the shortcut from the Startup folder
    wchar_t startupPath[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startupPath);
    std::wstring shortcutPath = std::wstring(startupPath) + L"\\MyApp.lnk";
    if (std::filesystem::exists(shortcutPath)) {
        std::filesystem::remove(shortcutPath);
    }
}


void SelfDestruct() {
    // Get the full path of the current executable
    char executablePath[MAX_PATH];
    GetModuleFileNameA(NULL, executablePath, MAX_PATH);

    // Create a temporary batch file
    std::ofstream batchFile("delete.bat");
    if (!batchFile) {
        std::cerr << "Could not create batch file.\n";
        return;
    }

    // Write commands to the batch file to delete the executable and then the batch file itself
    batchFile << "@echo off\n";
    // wait for 10 seconds for the application to exit
    batchFile << "timeout /T 10\n";
    batchFile << ":retry\n";
    // delete the current executable
    batchFile << "del \"" << executablePath << "\"\n";
    batchFile << "if exist \"" << executablePath << "\" goto retry\n";
    // Delete the batch file itself
    batchFile << "del \"%~f0\"\n";
    batchFile.close();

    // Start the batch file hidden using ShellExecute
    ShellExecuteA(NULL, "open", "delete.bat", NULL, NULL, SW_HIDE);

    // Exit the program
    exit(0);
}
