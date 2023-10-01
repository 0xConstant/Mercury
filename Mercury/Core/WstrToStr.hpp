#pragma once

#include <string>
#include <Windows.h>

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
