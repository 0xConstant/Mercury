#pragma once

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
#include "../Core/helpers.hpp"
#include "../Core/names.hpp"


#pragma comment(lib, "winhttp.lib")


std::string sendrequest(const std::wstring& fullUrl, const std::vector<char>& data, size_t startByte, size_t endByte, size_t totalFileSize) {
    std::wstring UID = GetUIDFromFile();

    DWORD dwSize = 0;
    DWORD dwDownloaded = 0;
    DWORD dwFlags = 0;
    DWORD dwTimeout = TIMEOUT;
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

    // Calculate Content-Range header value
    std::wstringstream contentRangeStream;
    contentRangeStream << L"bytes " << startByte << L"-" << endByte << L"/" << totalFileSize;
    const std::wstring contentRangeHeader = contentRangeStream.str();

    std::wstring headers = L"Content-Type: multipart/form-data; boundary=" + boundary +
        L"\r\nContent-Range: " + contentRangeHeader;

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

    // New: Add retry mechanism for interruptions
    const int maxRetries = RETRIES;  // Define maximum retries
    int retryCount = 0;
    BOOL resultSend = FALSE;
    BOOL resultReceive = FALSE;

    while (retryCount < maxRetries) {
        resultSend = WinHttpSendRequest(hRequest, headers.c_str(), static_cast<DWORD>(headers.size()), (LPVOID)fullData.data(), static_cast<DWORD>(fullData.size()), static_cast<DWORD>(fullData.size()), 0);
        if (!resultSend) {
            printf("Error in WinHttpSendRequest: %u\n", GetLastError());
        }
        else {
            resultReceive = WinHttpReceiveResponse(hRequest, NULL);
            if (!resultReceive) {
                printf("Error in WinHttpReceiveResponse: %u\n", GetLastError());
            }
            else {
                break;
            }
        }
        if (retryCount < (maxRetries - 1)) {  // No need to sleep after the last attempt
            Sleep(SLEEP);
        }
        retryCount++;
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

    size_t totalFileSize = GetTotalFileSize(filePath);

    for (size_t j = 0; j < miniChunks; ++j) {
        size_t miniStartByte = startByte + j * miniChunkSize;
        size_t miniEndByte = startByte + (j + 1) * miniChunkSize - 1;

        std::vector<char> miniBuffer(buffer.begin() + j * miniChunkSize, buffer.begin() + (j + 1) * miniChunkSize);
        // Send the miniBuffer using sendrequest()
        std::cout << "Sending chunk from byte " << miniStartByte << " to " << miniEndByte << std::endl;

        sendrequest(fullUrl, miniBuffer, miniStartByte, miniEndByte, totalFileSize); // Modified: Send mini-chunk with appropriate byte range
    }

    // Send remaining data
    size_t remainingStartByte = startByte + miniChunks * miniChunkSize;
    size_t remainingEndByte = endByte;

    std::vector<char> remainingBuffer(buffer.begin() + miniChunks * miniChunkSize, buffer.end());
    std::string response = sendrequest(fullUrl, remainingBuffer, remainingStartByte, remainingEndByte, totalFileSize); // Modified: Send remaining buffer with appropriate byte range
    return response != "ERROR";
}




size_t MeasureUploadSpeed(const std::wstring& speedTestUrl) {
    std::vector<char> testBuffer(10000);  // 10 KB buffer

    size_t startByte = 0;  // As it's a new buffer, starting from the beginning.
    size_t endByte = testBuffer.size() - 1;  // End byte would be the size of the buffer - 1.
    size_t totalFileSize = testBuffer.size();  // Total size is the size of our buffer.

    auto startTime = std::chrono::steady_clock::now();
    sendrequest(speedTestUrl, testBuffer, startByte, endByte, totalFileSize);  // Using the modified sendrequest
    auto endTime = std::chrono::steady_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime).count();
    return totalFileSize * 1000 / duration;  // Bytes per second
}




void UploadFile(const std::wstring& url, const std::string& path, const std::wstring& speedtesturl) {
    size_t totalFileSize = GetTotalFileSize(path);

    // Measure internet speed here using the provided url
    size_t internetSpeed = MeasureUploadSpeed(speedtesturl);

    // Modify to use 70% of the measured internet speed
    size_t desiredSpeed = internetSpeed * 0.7;  // Use 70% of measured internet speed

    for (size_t i = 0; i < totalFileSize; i += CHUNK_SIZE) {  // Start always from 0, resume functionality removed
        size_t startByte = i;
        size_t endByte = std::min(i + CHUNK_SIZE - 1, totalFileSize - 1);
        std::cout << "Uploading bytes from " << startByte << " to " << endByte << std::endl;

        // Passing the desiredSpeed to UploadFileChunk
        bool success = UploadFileChunk(path, url, startByte, endByte, desiredSpeed);

        if (success) {
            std::cout << "Successfully uploaded bytes " << endByte << "/" << totalFileSize << std::endl;
        }
        else {
            std::cout << "Failed to upload bytes from " << startByte << " to " << endByte << ". Retrying in 10 seconds." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));
            i -= CHUNK_SIZE;
        }
    }
}



