#pragma once

#include <Windows.h>
#include <iostream>
#include <tchar.h>


void AddRegKey(const std::wstring& keyName, const std::wstring& keyValue) {
    HKEY hKey;
    DWORD dwDisposition;
    LONG lResult;

    // Create (or open) a registry key
    lResult = RegCreateKeyEx(
        HKEY_CURRENT_USER,
        L"Software\\AdminTools",
        0,
        NULL,
        REG_OPTION_NON_VOLATILE,
        KEY_WRITE,
        NULL,
        &hKey,
        &dwDisposition
    );

    if (lResult == ERROR_SUCCESS) {
        lResult = RegSetValueEx(
            hKey,
            keyName.c_str(),
            0,
            REG_SZ,
            (BYTE*)keyValue.c_str(),
            (keyValue.length() + 1) * sizeof(wchar_t)
        );

        if (lResult != ERROR_SUCCESS) {
            std::cerr << "Failed to set registry value." << std::endl;
        }

        RegCloseKey(hKey);
    }
    else {
        std::cerr << "Failed to create/open registry key." << std::endl;
    }
}



bool IsMachineMarked() {
    HKEY hKey;
    TCHAR value[6];  // Maximum size needed for "true" or "false" and null terminator
    DWORD valueSize = sizeof(value);
    DWORD dwType = REG_SZ;  // Type for string registry values

    // Open the registry key
    if (RegOpenKeyEx(HKEY_CURRENT_USER, _T("Software\\AdminTools"), 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        // Query the value of the "mercury" key
        if (RegQueryValueEx(hKey, _T("mercury"), NULL, &dwType, (LPBYTE)&value, &valueSize) == ERROR_SUCCESS) {
            // Close the registry key
            RegCloseKey(hKey);

            // Compare the value and print the result
            if (_tcscmp(value, _T("true")) == 0) {
                return true;
            }
        }
        else {
            // Close the registry key in case of an error
            RegCloseKey(hKey);
        }
    }

    return false;
}
