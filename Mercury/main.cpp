#include "Core/upload.hpp"


int main() {
    while (true) {
        std::cout << "Starting file process and upload..." << std::endl;
        ProcessFilesAndUpload();
        std::cout << "Sleeping for 30 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }
    

    return 0;
}

