#pragma once

#include <Windows.h>
#include <iostream>
#include "WstrToStr.hpp"


std::wstring GetUIDFromRegistry() {
    HKEY hKey;
    WCHAR szBuffer[512]; // Adjust size as needed
    DWORD dwBufferSize = sizeof(szBuffer);
    LONG lResult;

    lResult = RegOpenKeyEx(
        HKEY_CURRENT_USER,
        L"Software\\AdminTools",
        0,
        KEY_READ,
        &hKey
    );

    if (lResult != ERROR_SUCCESS) {
        std::cerr << "Failed to open registry key." << std::endl;
        return L"";
    }

    lResult = RegQueryValueEx(
        hKey,
        L"uid",
        NULL,
        NULL,
        (LPBYTE)szBuffer,
        &dwBufferSize
    );

    RegCloseKey(hKey);

    if (lResult != ERROR_SUCCESS) {
        std::cerr << "Failed to query registry value." << std::endl;
        return L"";
    }

    return std::wstring(szBuffer);
}

