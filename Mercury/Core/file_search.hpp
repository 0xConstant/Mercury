#pragma once

#include <queue>
#include <unordered_set>
#include <thread>
#include <map>
#include <filesystem>
#include "helpers.hpp"


std::map<std::string, std::map<std::string, std::string>> FileSearcher(const std::wstring& dir, const std::vector<std::wstring>& fileTypes)
{
    std::map<std::string, std::map<std::string, std::string>> resultMap;

    std::queue<std::filesystem::path> todo;
    todo.push(dir);

    while (!todo.empty())
    {
        std::filesystem::path currentDir = todo.front();
        todo.pop();

        std::vector<std::filesystem::path> files;

        try
        {
            for (const auto& entry : std::filesystem::directory_iterator(currentDir))
            {
                if (entry.is_directory())
                {
                    todo.push(entry.path());
                }
                else if (entry.is_regular_file())
                {
                    files.push_back(entry.path());
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(2)); 
        }
        catch (const std::exception&) {}

        // THIS LOOP IS USED FOR SAVING FILE PATHS AND MODIFICATION DATE
        for (const auto& file : files) {
            if (std::find_if(fileTypes.begin(), fileTypes.end(), [&](const std::wstring& fileType) {
                return file.extension() == fileType;
                }) != fileTypes.end() && CheckPerms(file.wstring(), GENERIC_READ)) {

                auto mod_time = std::filesystem::last_write_time(file);
                auto mod_time_tp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
                    mod_time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now()
                );
                std::time_t mod_time_t = std::chrono::system_clock::to_time_t(mod_time_tp);

                std::string narrowPath = file.string();  // Direct conversion using std::filesystem
                resultMap[narrowPath]["date"] = std::to_string(static_cast<int64_t>(mod_time_t));
            }
        }
    }

    // Sort by modification time
    std::vector<std::pair<std::string, std::map<std::string, std::string>>> items(resultMap.begin(), resultMap.end());
    std::sort(items.begin(), items.end(), [](const auto& a, const auto& b) {
        return a.second.at("date") < b.second.at("date");
        });

    // Calculate size of all files and make sure they don't exceed MAX_SIZE_BYTES
    std::map<std::string, std::map<std::string, std::string>> sortedResults;
    std::int64_t totalSize = 0;
    for (const auto& item : items) {
        std::wstring filePath = std::wstring(item.first.begin(), item.first.end());
        std::int64_t fileSize = GetFileSize(filePath);
        if ((totalSize + fileSize) <= MAX_SIZE_BYTES) {
            totalSize += fileSize;
            sortedResults[item.first] = item.second;
        }
        else {
            // Stop adding files once the limit is reached
            break;
        }
    }

    return sortedResults;
}


