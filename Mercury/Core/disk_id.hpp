#pragma once

#include <map>
#include <vector>
#include <string>
#include <Windows.h>
#include "check_perms.hpp"


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

