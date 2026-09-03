#include <iostream>
#include <filesystem>
#include <vector>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <chrono>
#include <array>
#include <sstream>

namespace fs = std::filesystem;

struct ConversionJob {
    std::string chdFile;
    std::string isoFile;
    std::string cueFile;
    bool success = false;
};

std::mutex coutMutex;
std::mutex progressMutex;
std::condition_variable cv;

std::atomic<int> filesCompleted{ 0 };
int totalFilesInBatch = 0;

// Run chdman extractcd for one file, suppress output, set success flag
void convertChdToIso(ConversionJob& job) {
    std::ostringstream cmd;
    // chdman extractcd -i "input.chd" -o "output.cue" -ob "output.iso"
    cmd << "chdman extractcd -i \"" << job.chdFile << "\" -o \"" << job.cueFile
        << "\" -ob \"" << job.isoFile << "\" >nul 2>&1";

    int ret = std::system(cmd.str().c_str());
    job.success = (ret == 0);
    filesCompleted++;

    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[Thread] " << (job.success ? "Success: " : "Failure: ") << job.chdFile << std::endl;
    }

    cv.notify_one();
}

// Wait for all threads in vector to complete (join)
void joinAllThreads(std::vector<std::thread>& threads) {
    for (auto& t : threads) {
        if (t.joinable())
            t.join();
    }
    threads.clear();
}

void printBatchFiles(const std::vector<ConversionJob>& batch) {
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "Converting batch:\n";
    for (const auto& job : batch) {
        std::cout << "  " << job.chdFile << "\n";
    }
}

void printProgress() {
    while (true) {
        std::unique_lock<std::mutex> lock(progressMutex);
        cv.wait_for(lock, std::chrono::milliseconds(250));

        int completed = filesCompleted.load();
        float percent = 0;
        if (totalFilesInBatch > 0)
            percent = (completed * 100.0f) / totalFilesInBatch;

        {
            std::lock_guard<std::mutex> coutLock(coutMutex);
            std::cout << "\rBatch progress: " << (int)percent << "% completed.    " << std::flush;
        }

        if (completed >= totalFilesInBatch) {
            std::lock_guard<std::mutex> coutLock(coutMutex);
            std::cout << std::endl; // Move to next line after 100%
            break;
        }
    }
}

int main() {
    std::cout << "Scanning for CHD files...\n";

    std::vector<std::string> chdFiles;
    for (auto& entry : fs::directory_iterator(fs::current_path())) {
        if (entry.is_regular_file()) {
            auto ext = entry.path().extension().string();
            for (auto& c : ext) c = tolower(c);
            if (ext == ".chd") {
                chdFiles.push_back(entry.path().string());
            }
        }
    }

    if (chdFiles.empty()) {
        std::cout << "No CHD files found to process.\n";
        return 0;
    }

    std::cout << "Found " << chdFiles.size() << " CHD files to process.\n";

    const size_t batchSize = 4;
    size_t totalFiles = chdFiles.size();
    size_t processedFiles = 0;

    while (processedFiles < totalFiles) {
        size_t count = std::min(batchSize, totalFiles - processedFiles);

        // Prepare batch
        std::vector<ConversionJob> batch;
        for (size_t i = 0; i < count; i++) {
            std::string chdPath = chdFiles[processedFiles + i];
            std::string baseName = fs::path(chdPath).stem().string();
            std::string isoPath = baseName + ".iso";
            std::string cuePath = baseName + ".cue";

            batch.push_back({ chdPath, isoPath, cuePath, false });
        }

        printBatchFiles(batch);

        filesCompleted = 0;
        totalFilesInBatch = static_cast<int>(count);

        // Launch progress thread
        std::thread progressThread(printProgress);

        // Launch conversion threads (max batchSize)
        std::vector<std::thread> workers;
        for (auto& job : batch) {
            workers.emplace_back(convertChdToIso, std::ref(job));
        }

        // Wait all conversions done
        joinAllThreads(workers);
        progressThread.join();

        // Run ps2_chd.exe to rename CHDs (assuming ps2_chd.exe is in current directory)
        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Running ps2.exe to rename converted ISOs...\n";
        }
        int renameRet = std::system("ps2_chd.exe");
        if (renameRet != 0) {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "Warning: ps2_chd.exe returned error code " << renameRet << "\n";
        }

        // Clean up ISO and CUE files from this batch after rename
        for (const auto& job : batch) {
            if (job.success) {
                std::error_code ec;
                if (fs::exists(job.isoFile) && !fs::remove(job.isoFile, ec)) {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "Warning: Failed to delete ISO file \"" << job.isoFile << "\": " << ec.message() << "\n";
                }
                if (fs::exists(job.cueFile) && !fs::remove(job.cueFile, ec)) {
                    std::lock_guard<std::mutex> lock(coutMutex);
                    std::cout << "Warning: Failed to delete CUE file \"" << job.cueFile << "\": " << ec.message() << "\n";
                }
            }
        }

        processedFiles += count;

        {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << std::endl;
        }
    }

    std::cout << "All CHD files processed.\n";

    return 0;
}
