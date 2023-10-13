#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <iostream>
#include <filesystem>
#include "helpers.hpp"
#include "minizip/zip.h"


const int ZIP_SUCCESS = 0;


int ZipFiles(const std::wstring& destinationPath, const std::vector<std::wstring>& paths)
{
    zipFile zf = zipOpen(std::string(destinationPath.begin(), destinationPath.end()).c_str(), APPEND_STATUS_CREATE);
    if (zf == NULL)
    {
        std::cerr << "Failed to open ZIP destination." << std::endl;
        return 1;
    }

    for (const auto& path : paths)
    {
        // Check file existence and read permissions
        if (!std::filesystem::exists(path) || !CheckPerms(path, GENERIC_READ))
        {
            std::cerr << "File does not exist or lacks read permissions: " << std::string(path.begin(), path.end()) << std::endl;
            continue; // Skip the current iteration
        }

        std::ifstream file(path, std::ios::binary | std::ios::in);
        if (file.is_open())
        {
            file.seekg(0, std::ios::end);
            long size = file.tellg();
            file.seekg(0, std::ios::beg);

            std::vector<char> buffer(size);
            if (size == 0 || file.read(buffer.data(), size))
            {
                zip_fileinfo zfi = { 0 };

                // Use the full path as the file name in the ZIP
                std::string zipPath = std::string(path.begin(), path.end());
                std::replace(zipPath.begin(), zipPath.end(), '\\', '/'); // Replace backslashes with forward slashes

                if (ZIP_SUCCESS != zipOpenNewFileInZip(zf, zipPath.c_str(), &zfi, NULL, 0, NULL, 0, NULL, Z_DEFLATED, Z_DEFAULT_COMPRESSION))
                {
                    std::cerr << "Failed to open new file in ZIP: " << zipPath << std::endl;
                }
                else if (zipWriteInFileInZip(zf, size == 0 ? "" : buffer.data(), size))
                {
                    std::cerr << "Failed to write data to ZIP: " << zipPath << std::endl;
                }
                else if (zipCloseFileInZip(zf))
                {
                    std::cerr << "Failed to close file in ZIP: " << zipPath << std::endl;
                }
                file.close();
            }
            else
            {
                std::cerr << "Failed to process the file: " << std::string(path.begin(), path.end()) << std::endl;
            }
        }
    }

    if (zipClose(zf, NULL))
    {
        std::cerr << "Failed to close ZIP destination." << std::endl;
        return 3;
    }

    return ZIP_SUCCESS;
}


