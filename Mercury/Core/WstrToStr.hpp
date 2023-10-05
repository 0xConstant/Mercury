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

