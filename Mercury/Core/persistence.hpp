#pragma once

#include <iostream>
#include <fstream>
#include <Windows.h>
#include <ShlObj.h>
#include <chrono>
#include <thread>
#include <filesystem>



void CreateStartupShortcut() {
    // Get the full path to the current application
    wchar_t appPath[MAX_PATH];
    GetModuleFileNameW(NULL, appPath, MAX_PATH);

    // Get the path to the Startup folder
    wchar_t startupPath[MAX_PATH];
    SHGetFolderPathW(NULL, CSIDL_STARTUP, NULL, 0, startupPath);

    // Get the current directory to form the full path to the VBScript
    wchar_t currentDirectory[MAX_PATH];
    GetCurrentDirectoryW(MAX_PATH, currentDirectory);
    std::wstring tempVbsFile = std::wstring(currentDirectory) + L"\\create_shortcut.vbs";

    // Write the VBScript
    std::wofstream vbsFile(tempVbsFile);
    vbsFile << L"Set objShell = CreateObject(\"WScript.Shell\")" << std::endl;
    vbsFile << L"strDesktop = \"" << startupPath << L"\"" << std::endl;
    vbsFile << L"Set objShortcut = objShell.CreateShortcut(strDesktop & \"\\MyApp.lnk\")" << std::endl;
    vbsFile << L"objShortcut.TargetPath = \"" << appPath << L"\"" << std::endl;
    vbsFile << L"objShortcut.Save" << std::endl;
    vbsFile.close();

    // Execute the VBScript
    ShellExecuteW(NULL, L"open", L"wscript.exe", tempVbsFile.c_str(), NULL, SW_HIDE);

    // Delete the VBScript
    std::this_thread::sleep_for(std::chrono::seconds(2)); // Give it a moment to run the script before deletion.
    std::filesystem::remove(tempVbsFile.c_str());
}


