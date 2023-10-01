#pragma once
#include <string>
#include <vector>
#include <ShlObj.h>



std::wstring GetFolderPath(const KNOWNFOLDERID& folderId) {
    wchar_t* pathRaw = nullptr;
    if (SHGetKnownFolderPath(folderId, 0, nullptr, &pathRaw) == S_OK) {
        std::wstring path(pathRaw);
        CoTaskMemFree(pathRaw);
        return path;
    }
    else {
        return L"";
    }
}


std::vector<std::wstring> GetSpecialPaths() {
    std::vector<std::wstring> paths;

    paths.push_back(GetFolderPath(FOLDERID_Desktop));
    paths.push_back(GetFolderPath(FOLDERID_Documents));
    paths.push_back(GetFolderPath(FOLDERID_Downloads));
    paths.push_back(GetFolderPath(FOLDERID_Pictures));
    paths.push_back(GetFolderPath(FOLDERID_SkyDrive));

    return paths;
}