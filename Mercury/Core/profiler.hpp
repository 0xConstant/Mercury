#pragma once

#include <map>
#include <string>
#include <chrono>
#include <ctime>
#include <functional>
#include <windows.h>
#include <lm.h>
#include <iostream>
#include "WstrToStr.hpp"


#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "ws2_32.lib")


bool IsMachineInAD() {
	LPWSTR lpNameBuffer = NULL;
	NETSETUP_JOIN_STATUS BufferType;

	if (NetGetJoinInformation(NULL, &lpNameBuffer, &BufferType) != NERR_Success)
		return false;

	if (BufferType == NetSetupDomainName)
		return true;

	if (lpNameBuffer)
		NetApiBufferFree(lpNameBuffer);

	return false;
}


std::string GetFQDN() {
	WCHAR buffer[MAX_PATH];
	DWORD size = sizeof(buffer) / sizeof(WCHAR);

	if (GetComputerNameEx(ComputerNameDnsFullyQualified, buffer, &size))
		return WStringToString(buffer);

	return "";
}


std::string GetDomainName() {
	CHAR buffer[MAX_PATH];
	DWORD size = sizeof(buffer) / sizeof(CHAR);

	if (GetComputerNameExA(ComputerNameDnsDomain, buffer, &size)) {
		std::wstring wDomainName(buffer, buffer + size);
		return WStringToString(wDomainName);
	}
		
	return "";
}


std::string GetLocalIP() {
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		return "";

	char hostName[255];
	gethostname(hostName, sizeof(hostName));
	struct hostent* host = gethostbyname(hostName);

	std::string ip = inet_ntoa(*reinterpret_cast<struct in_addr*>(host->h_addr_list[0]));

	WSACleanup();
	return ip;
}


std::wstring GetCurrentUsername() {
	DWORD bufferSize = 0;
	GetUserNameW(NULL, &bufferSize);  // Get required buffer size

	std::wstring username(bufferSize, L'\0');
	if (!GetUserNameW(&username[0], &bufferSize)) {
		return L"";
	}

	// Remove the null character that GetUserNameW might add.
	username.erase(std::find(username.begin(), username.end(), L'\0'), username.end());

	return username;
}


std::vector<std::string> GetUserLocalGroups() {

	std::wstring username = GetCurrentUsername();
	if (username.empty()) {
		//std::cerr << "Failed to get the current username." << std::endl;
		return {};
	}

	std::vector<std::string> groupNames;
	DWORD dwLevel = 0;
	DWORD dwFlags = LG_INCLUDE_INDIRECT;
	LPWSTR user = const_cast<LPWSTR>(username.c_str());
	LPLOCALGROUP_USERS_INFO_0 pBuf = NULL;
	DWORD dwEntriesRead = 0;
	DWORD dwTotalEntries = 0;

	NET_API_STATUS nStatus = NetUserGetLocalGroups(NULL, user, dwLevel, dwFlags, (LPBYTE*)&pBuf, MAX_PREFERRED_LENGTH, &dwEntriesRead, &dwTotalEntries);
	if (nStatus == NERR_Success) {
		for (DWORD i = 0; i < dwEntriesRead; i++) {
			if (pBuf[i].lgrui0_name) {
				groupNames.push_back(WStringToString(pBuf[i].lgrui0_name));
			}
		}
	}

	if (pBuf != NULL) {
		NetApiBufferFree(pBuf);
	}

	return groupNames;
}


std::vector<std::string> GetUserADGroups() {

	std::wstring username = GetCurrentUsername();
	if (username.empty()) {
		//std::cerr << "Failed to get the current username." << std::endl;
		return {};
	}

	std::vector<std::string> groupNames;
	DWORD dwLevel = 0;
	LPWSTR user = const_cast<LPWSTR>(username.c_str());
	LPGROUP_USERS_INFO_0 pBuf = NULL;
	DWORD dwEntriesRead = 0;
	DWORD dwTotalEntries = 0;

	NET_API_STATUS nStatus = NetUserGetGroups(NULL, user, dwLevel, (LPBYTE*)&pBuf, MAX_PREFERRED_LENGTH, &dwEntriesRead, &dwTotalEntries);
	if (nStatus == NERR_Success) {
		for (DWORD i = 0; i < dwEntriesRead; i++) {
			if (pBuf[i].grui0_name) {
				groupNames.push_back(WStringToString(pBuf[i].grui0_name));
			}
		}
	}

	if (pBuf != NULL) {
		NetApiBufferFree(pBuf);
	}

	return groupNames;
}


std::string VectorToString(const std::vector<std::string>& vec) {
	std::string result = "[";
	for (const auto& str : vec) {
		result += "\"" + str + "\", ";
	}
	if (vec.size() > 0) {
		result.pop_back();  // Remove trailing comma
		result.pop_back();  // Remove trailing space
	}
	result += "]";
	return result;
}


std::string GetUUID() {
	char uuid[100];
	SECURITY_ATTRIBUTES sa;
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	HANDLE hRead, hWrite;
	if (!CreatePipe(&hRead, &hWrite, &sa, 0)) {
		return "";
	}

	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
	si.cb = sizeof(si);
	si.hStdError = hWrite;
	si.hStdOutput = hWrite;
	si.hStdInput = hRead;
	si.dwFlags |= STARTF_USESTDHANDLES | STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_HIDE;

	ZeroMemory(&pi, sizeof(pi));

	WCHAR cmd[] = L"wmic csproduct get uuid";
	if (!CreateProcess(NULL, cmd, NULL, NULL, TRUE, 0, NULL, NULL, &si, &pi)) {
		CloseHandle(hWrite);
		CloseHandle(hRead);
		return "";
	}

	WaitForSingleObject(pi.hProcess, INFINITE);
	DWORD bytesRead;
	if (!ReadFile(hRead, uuid, sizeof(uuid) - 1, &bytesRead, NULL) || bytesRead == 0) {
		CloseHandle(hWrite);
		CloseHandle(hRead);
		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
		return "";
	}

	CloseHandle(hWrite);
	CloseHandle(hRead);
	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	std::string uuidStr(uuid);
	uuidStr.erase(std::remove(uuidStr.begin(), uuidStr.end(), '\n'), uuidStr.end());

	// Remove "UUID" prefix and any leading whitespace
	std::size_t pos = uuidStr.find("UUID");
	if (pos != std::string::npos) {
		uuidStr = uuidStr.substr(pos + 4); // Removes "UUID"
	}

	// Trim leading spaces.
	uuidStr.erase(uuidStr.begin(), std::find_if(uuidStr.begin(), uuidStr.end(), [](unsigned char ch) {
		return !std::isspace(ch);
		}));

	return uuidStr;
}


void SaveUIDToFile(const std::wstring& uid) {
	std::wofstream outfile(L".uid");  // Open the .uid file for writing

	if (outfile.is_open()) {
		outfile << uid;
		outfile.close();
	}
	else {
		//std::cerr << "Failed to open .uid file for writing." << std::endl;
	}
}



std::map<std::string, std::string> Profiler() {
	std::map<std::string, std::string> data;
	char buffer[256];
	DWORD size = 256;

	if (GetComputerNameA(buffer, &size)) {
		data["hostname"] = buffer;
	}

	if (GetUserNameA(buffer, &size)) {
		data["username"] = buffer;
	}

	// Getting current time as a string
	auto now = std::chrono::system_clock::now();
	std::time_t currentTime = std::chrono::system_clock::to_time_t(now);
	std::tm localTimeStruct;
	localtime_s(&localTimeStruct, &currentTime);
	char timeStr[100];
	strftime(timeStr, sizeof(timeStr), "%Y-%m-%d %H:%M:%S", &localTimeStruct);

	// Hashing the time string
	std::string serialNumber = GetUUID();
	if (!serialNumber.empty()) {
		std::hash<std::string> stringHasher;
		data["uid"] = std::to_string(stringHasher(timeStr)); // Convert hash value to string
	}
	data["uid"] = serialNumber;
	SaveUIDToFile(StringToWString(serialNumber));

	if (IsMachineInAD()) {
		data["fqdn"] = GetFQDN();
		data["domain"] = GetDomainName();
		data["local_ip"] = GetLocalIP();
		data["local_groups"] = VectorToString(GetUserLocalGroups());
		data["ad_groups"] = VectorToString(GetUserADGroups());
	}


	return data;  // Return the map
}
