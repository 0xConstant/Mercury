#define NOMINMAX
#include <windows.h>
#include <winhttp.h>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>
#include <chrono>
#include <sstream>
#include <chrono>
#include "../Core/get_uid.hpp"

#pragma comment(lib, "winhttp.lib")

static constexpr size_t CHUNK_SIZE = 1000 * 1024; // 1MB
std::wstring UID = GetUIDFromRegistry();


size_t GetTotalFileSize(const std::string& filePath) {
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    return file.tellg();
}


std::string sendrequest(const std::wstring& fullUrl, const std::vector<char>& data) {
    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    DWORD dwFlags = 0;
    DWORD dwTimeout = 10 * 1000;
    HINTERNET hSession = NULL, hConnect = NULL, hRequest = NULL;
    BOOL  bResults = FALSE;
    std::string response;

    URL_COMPONENTS urlComponents;
    ZeroMemory(&urlComponents, sizeof(urlComponents));
    urlComponents.dwStructSize = sizeof(urlComponents);
    urlComponents.dwSchemeLength = (DWORD)-1;
    urlComponents.dwHostNameLength = (DWORD)-1;
    urlComponents.dwUrlPathLength = (DWORD)-1;
    urlComponents.dwExtraInfoLength = (DWORD)-1;

    wchar_t hostName[256];
    wchar_t urlPath[256];
    urlComponents.lpszHostName = hostName;
    urlComponents.lpszUrlPath = urlPath;
    urlComponents.dwHostNameLength = 256;
    urlComponents.dwUrlPathLength = 256;

    if (!WinHttpCrackUrl(fullUrl.c_str(), (DWORD)wcslen(fullUrl.c_str()), 0, &urlComponents)) {
        printf("Error %u in WinHttpCrackUrl.\n", GetLastError());
        return response;
    }

    hSession = WinHttpOpen(L"WinHTTP FileUploader/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (hSession)
        hConnect = WinHttpConnect(hSession, urlComponents.lpszHostName, urlComponents.nPort, 0);
    if (hConnect)
        hRequest = WinHttpOpenRequest(hConnect, L"POST", urlComponents.lpszUrlPath, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, urlComponents.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0);

    if (hRequest) {
        WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &dwTimeout, sizeof(dwTimeout));
        dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
    }

    std::wstringstream wss;
    wss << GetTickCount64();
    const std::wstring boundary = L"----WinHTTPBoundary" + wss.str();
    std::wstring headers = L"Content-Type: multipart/form-data; boundary=" + boundary;
    std::wstring preFileData = L"--" + boundary + L"\r\n" +
        L"Content-Disposition: form-data; name=\"file\"; filename=\"chunk.data\"\r\n" +
        L"Content-Type: application/octet-stream\r\n\r\n";

    std::wstring uidData = L"--" + boundary + L"\r\n" +
        L"Content-Disposition: form-data; name=\"uid\"\r\n\r\n" + UID + L"\r\n";
    std::wstring postFileData = L"\r\n--" + boundary + L"--\r\n";

    std::vector<char> fullData;
    fullData.insert(fullData.end(), uidData.begin(), uidData.end());
    fullData.insert(fullData.end(), preFileData.begin(), preFileData.end());
    fullData.insert(fullData.end(), data.begin(), data.end());
    fullData.insert(fullData.end(), postFileData.begin(), postFileData.end());



    BOOL resultSend = WinHttpSendRequest(hRequest, headers.c_str(), static_cast<DWORD>(headers.size()), (LPVOID)fullData.data(), static_cast<DWORD>(fullData.size()), static_cast<DWORD>(fullData.size()), 0);
    if (!resultSend) {
        printf("Error in WinHttpSendRequest: %u\n", GetLastError());
    }

    BOOL resultReceive = WinHttpReceiveResponse(hRequest, NULL);
    if (!resultReceive) {
        printf("Error in WinHttpReceiveResponse: %u\n", GetLastError());
    }

    if (resultSend && resultReceive) {
        do {
            dwSize = 0;
            if (!WinHttpQueryDataAvailable(hRequest, &dwSize)) {
                printf("Error %u in WinHttpQueryDataAvailable.\n", GetLastError());
                break;
            }

            std::vector<char> buffer(dwSize);
            if (!WinHttpReadData(hRequest, (LPVOID)buffer.data(), dwSize, &dwDownloaded)) {
                printf("Error %u in WinHttpReadData.\n", GetLastError());
                break;
            }
            else {
                response.append(buffer.data(), dwDownloaded);
            }
        } while (dwSize > 0);
    }



    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);

    return response;
}



bool UploadFileChunk(const std::string& filePath, const std::wstring& fullUrl, size_t startByte, size_t endByte, size_t bytesPerSecond) {
    std::ifstream file(filePath, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open file." << std::endl;
        return false;
    }

    file.seekg(startByte, std::ios::beg);
    std::vector<char> buffer(endByte - startByte + 1);
    file.read(buffer.data(), buffer.size());
    file.close();

    // Calculate the number of mini-chunks and the time to sleep after sending each mini-chunk
    const size_t miniChunkSize = bytesPerSecond; // We send this amount of data per second
    size_t miniChunks = buffer.size() / miniChunkSize;
    int sleepDuration = 1000; // Sleep for 1 second after sending each mini-chunk

    for (size_t j = 0; j < miniChunks; ++j) {
        std::vector<char> miniBuffer(buffer.begin() + j * miniChunkSize, buffer.begin() + (j + 1) * miniChunkSize);
        // Send the miniBuffer using sendrequest() 
        sendrequest(fullUrl, miniBuffer); // Modification: Send mini-chunk here
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepDuration));
    }

    // Send remaining data
    std::vector<char> remainingBuffer(buffer.begin() + miniChunks * miniChunkSize, buffer.end());
    std::string response = sendrequest(fullUrl, remainingBuffer); // Modification: Send remaining buffer here
    return response != "ERROR";
}



size_t MeasureUploadSpeed(const std::wstring& speedTestUrl) {
    std::vector<char> testBuffer(10000);  // 10 KB buffer
    auto startTime = std::chrono::steady_clock::now();
    sendrequest(speedTestUrl, testBuffer);
    auto endTime = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    return testBuffer.size() * 1000 / duration;  // Bytes per second
}



void UploadFile(const std::wstring& url, const std::string& path) {
    size_t totalFileSize = GetTotalFileSize(path);
    size_t lastSuccessfulChunkEnd = 0;

    std::string successFlagFilePath = path + ".upload_success";
    std::ifstream successFlagFile(successFlagFilePath, std::ios::in);

    if (successFlagFile.is_open()) {
        successFlagFile >> lastSuccessfulChunkEnd;
        successFlagFile.close();
    }

    // Measure internet speed here using the provided url
    std::wstring speedtesturl = L"https://10.0.0.113:5000/speedtest";
    size_t internetSpeed = MeasureUploadSpeed(speedtesturl);
    size_t desiredSpeed = internetSpeed / 3;  // Use one third of measured internet speed

    for (size_t i = lastSuccessfulChunkEnd; i < totalFileSize; i += CHUNK_SIZE) {
        size_t startByte = i;
        size_t endByte = std::min(i + CHUNK_SIZE - 1, totalFileSize - 1);
        std::cout << "Uploading bytes from " << startByte << " to " << endByte << std::endl;

        // Passing the desiredSpeed to UploadFileChunk
        bool success = UploadFileChunk(path, url, startByte, endByte, desiredSpeed);

        if (success) {
            std::cout << "Successfully uploaded bytes " << endByte << "/" << totalFileSize << std::endl;
            std::ofstream successFlagFileOut(successFlagFilePath, std::ios::out);
            if (successFlagFileOut.is_open()) {
                successFlagFileOut << endByte;
                successFlagFileOut.close();
            }
        }
        else {
            std::cout << "Failed to upload bytes from " << startByte << " to " << endByte << ". Retrying in 10 seconds." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
            i -= CHUNK_SIZE;
        }
    }

    std::remove(successFlagFilePath.c_str());
}


