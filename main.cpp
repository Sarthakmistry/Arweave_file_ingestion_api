#include <iostream>
#include <string>
#include <array>
#include <memory>
#include <stdexcept>
#include <curl/curl.h>

// Callback function to capture HTTP response data
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Executes the Node.js script and reads the console output (the TxID)
std::string executeNodeScript(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    
#ifdef _WIN32
    std::unique_ptr<FILE, decltype(&_pclose)> pipe(_popen(cmd, "r"), _pclose);
#else
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
#endif

    if (!pipe) throw std::runtime_error("popen() failed to start script!");
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// Reads the file content back from the local Arweave testnet
std::string readFromArweave(const std::string& txId) {
    CURL* curl = curl_easy_init();
    std::string readBuffer;
    if(curl) {
        // Point to the local testnet
        std::string url = "http://localhost:1984/" + txId; 
        
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        
        curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    return readBuffer;
}

int main() {
    curl_global_init(CURL_GLOBAL_DEFAULT);

    // The dummy file we are going to upload
    std::string filePath = "test_data.txt";
    std::string command = "node arweave_upload.js " + filePath;

    std::cout << "--- Step 1: Uploading file to ArLocal via Node.js ---" << std::endl;
    std::string txId = executeNodeScript(command.c_str());
    
    // Arweave Transaction IDs are exactly 43 characters long
    if (txId.length() >= 43 && txId.find("failed") == std::string::npos) {
        std::cout << "Success! File uploaded." << std::endl;
        std::cout << "Transaction ID: " << txId << std::endl;
        
        std::cout << "\n--- Step 2: Reading file back natively in C++ ---" << std::endl;
        std::string content = readFromArweave(txId);
        std::cout << "Retrieved Content: " << content << std::endl;
        
        std::cout << "\nYou can also view this file in your browser at:" << std::endl;
        std::cout << "http://localhost:1984/" << txId << std::endl;
    } else {
        std::cerr << "Upload failed. Output: " << txId << std::endl;
    }

    curl_global_cleanup();
    return 0;
}