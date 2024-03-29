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


std::string SendData(const std::wstring& method,
    const std::wstring& fullUrl,
    const std::wstring& headers = L"",
    const nlohmann::json& data = nlohmann::json::object(),
    const std::wstring& uid = L"") {

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

    // Parse the URL using WinHttpCrackUrl
    if (!WinHttpCrackUrl(fullUrl.c_str(), (DWORD)wcslen(fullUrl.c_str()), 0, &urlComponents)) {
        printf("Error %u in WinHttpCrackUrl.\n", GetLastError());
        return response;
    }

    // Initialize the use of WinHTTP function
    hSession = WinHttpOpen(L"Mercury Uploader", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, 
        WINHTTP_NO_PROXY_NAME, 
        WINHTTP_NO_PROXY_BYPASS, 0);

    // Establish a connection to an HTTP server 
    if (hSession)
        hConnect = WinHttpConnect(hSession, urlComponents.lpszHostName, urlComponents.nPort, 0);

    // Create an HTTP request handle, specify path, request type
    if (hConnect)
        hRequest = WinHttpOpenRequest(hConnect, method.c_str(),
            urlComponents.lpszUrlPath, NULL, WINHTTP_NO_REFERER,
            WINHTTP_DEFAULT_ACCEPT_TYPES,
            urlComponents.nScheme == INTERNET_SCHEME_HTTPS ? WINHTTP_FLAG_SECURE : 0); 

    // Set timeout and ignore insecure or invalid SSL certificate error 
    if (hRequest) {
        WinHttpSetOption(hRequest, WINHTTP_OPTION_RECEIVE_TIMEOUT, &dwTimeout, sizeof(dwTimeout));
        dwFlags |= SECURITY_FLAG_IGNORE_UNKNOWN_CA | SECURITY_FLAG_IGNORE_CERT_WRONG_USAGE | SECURITY_FLAG_IGNORE_CERT_CN_INVALID | SECURITY_FLAG_IGNORE_CERT_DATE_INVALID;
        WinHttpSetOption(hRequest, WINHTTP_OPTION_SECURITY_FLAGS, &dwFlags, sizeof(dwFlags));
    }

    std::vector<char> fullData;

    if (!data.empty() && !uid.empty()) {
        std::wstringstream wss;
        wss << GetTickCount64();
        const std::wstring boundary = L"----WinHTTPBoundary" + wss.str();

        std::wstring preFileData = L"--" + boundary + L"\r\n" +
            L"Content-Disposition: form-data; name=\"file\"; filename=\"chunk.data\"\r\n" +
            L"Content-Type: application/octet-stream\r\n\r\n";

        std::wstring uidData = L"--" + boundary + L"\r\n" +
            L"Content-Disposition: form-data; name=\"uid\"\r\n\r\n" + uid + L"\r\n";
        std::wstring postFileData = L"\r\n--" + boundary + L"--\r\n";

        fullData.insert(fullData.end(), uidData.begin(), uidData.end());
        fullData.insert(fullData.end(), preFileData.begin(), preFileData.end());

        std::string jsonData = data.dump();
        fullData.insert(fullData.end(), jsonData.begin(), jsonData.end());

        fullData.insert(fullData.end(), postFileData.begin(), postFileData.end());
    }
    else {
        std::string jsonData = data.dump();
        fullData.insert(fullData.end(), jsonData.begin(), jsonData.end());
    }

    // Connection retry mechanism for interruptions
    const int maxRetries = RETRIES;
    BOOL resultSend = FALSE;
    BOOL resultReceive = FALSE;

    // Add headers to specify we are sending JSON data:
    if (hRequest) {
        LPCWSTR additionalHeaders = L"Content-Type: application/json";
        bResults = WinHttpAddRequestHeaders(hRequest, additionalHeaders, wcslen(additionalHeaders), WINHTTP_ADDREQ_FLAG_ADD);
        if (!bResults) {
            printf("Error %u in WinHttpAddRequestHeaders.\n", GetLastError());
            goto cleanup;
        }
    }

    // Retry loop
    while (true) {
        // Retry loop for each phase
        for (int retryCount = 0; retryCount < maxRetries; retryCount++) {
            resultSend = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, (LPVOID)fullData.data(), static_cast<DWORD>(fullData.size()), static_cast<DWORD>(fullData.size()), 0);
            if (!resultSend) {
                printf("[ POST request failed ] Error in WinHttpSendRequest: %u\n", GetLastError());
            }
            else {
                resultReceive = WinHttpReceiveResponse(hRequest, NULL);
                if (!resultReceive) {
                    printf("[ Unable to receive a response ] Error in WinHttpReceiveResponse: %u\n", GetLastError());
                }
                else {
                    break;
                }
            }
            // sleep between each connection retry
            if (retryCount < (maxRetries - 1)) {
                Sleep(SLEEP);
            }
        }
        // Check if both conditions succeeded after the for loop, and if so, break from the outer while loop
        if (resultSend && resultReceive) {
            break;
        }
        else {
            // After exhausting all retries for this phase, sleep for 60 seconds
            printf("All %d upload attempts failed.\n", RETRIES);
            Sleep(60000);
            std::cout << "Retrying upload again..." << std::endl;
        }
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

cleanup:
    if (hRequest) WinHttpCloseHandle(hRequest);
    if (hConnect) WinHttpCloseHandle(hConnect);
    if (hSession) WinHttpCloseHandle(hSession);


    return response;
}



