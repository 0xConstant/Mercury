#pragma once

#include <windows.h>
#include <iostream>
#include <iphlpapi.h>
#include <icmpapi.h>

#pragma comment(lib, "iphlpapi.lib")
#pragma comment(lib, "ws2_32.lib")


bool googleConn() {
    HANDLE hIcmp;
    unsigned long ipaddr = INADDR_NONE;
    DWORD dwRetVal = 0;
    char SendData[] = "Data Buffer";
    LPVOID ReplyBuffer = nullptr;
    DWORD ReplySize = 0;
    int timeout = 10000; // 10 seconds
    const int retry = 3;

    ipaddr = inet_addr("8.8.8.8");  // Google DNS
    hIcmp = IcmpCreateFile();
    if (hIcmp == INVALID_HANDLE_VALUE) {
        std::cerr << "IcmpCreateFile failed: " << GetLastError() << std::endl;
        return false;
    }

    ReplySize = sizeof(ICMP_ECHO_REPLY) + sizeof(SendData);
    ReplyBuffer = (VOID*)malloc(ReplySize);
    if (ReplyBuffer == nullptr) {
        std::cerr << "Unable to allocate memory" << std::endl;
        return false;
    }

    while (true) {
        bool success = false;
        for (int i = 0; i < retry; i++) {
            dwRetVal = IcmpSendEcho(hIcmp, ipaddr, SendData, sizeof(SendData),
                nullptr, ReplyBuffer, ReplySize, timeout);

            std::cout << "Attempt #" << i + 1 << std::endl;  // Print out the attempt number

            if (dwRetVal != 0) {
                PICMP_ECHO_REPLY pingReply = (PICMP_ECHO_REPLY)ReplyBuffer;
                if (pingReply->Status == IP_SUCCESS) {
                    success = true;
                    break;
                }
                else {
                    std::cerr << "Ping returned with status: " << pingReply->Status << std::endl;
                }
            }
            else {
                std::cerr << "IcmpSendEcho failed." << std::endl;  // Print if IcmpSendEcho itself failed
            }
        }

        if (success) {
            free(ReplyBuffer);
            return true;
        }
        else {
            std::cout << "Trying again in 20 seconds..." << std::endl;
            Sleep(20000); // Sleep for 20 seconds after 3 failed attempts
        }
    }


    // This line will technically never be reached, but it's good form to free resources.
    free(ReplyBuffer);
    return false;
}
