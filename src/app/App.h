#pragma once

#include "core/Database.h"
#include "core/Renamer.h"

#include <SDL3/SDL.h>

#include <atomic>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ps2br {

class App {
public:
    App();
    ~App();

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    bool initialize();
    int run();

private:
    enum class Filter {
        All,
        Ready,
        Renamed,
        Failed,
        Skipped,
    };

    enum class Operation {
        None,
        Scanning,
        Renaming,
    };

    static void SDLCALL folderDialogCallback(void* userdata, const char* const* files, int filter);
    static void SDLCALL databaseDialogCallback(void* userdata, const char* const* files, int filter);
    static void SDLCALL reportDialogCallback(void* userdata, const char* const* files, int filter);

    void processEvents(bool& done);
    void processDialogResults();
    void draw();
    void drawHeader();
    void drawSetupPanel();
    void drawProgressPanel();
    void drawResultsPanel();
    void drawActivityPanel();
    void drawFailurePopup();
    void drawAboutPopup();
    void drawStatusBadge(ResultStatus status) const;

    void chooseFolder();
    void chooseDatabase();
    void chooseReportDestination();
    void resetDatabase();
    void startScan();
    void startRename();
    void requestCancel();
    void reapWorker();
    void invalidatePreview(std::string reason);

    void appendActivity(std::string message);
    void updateResult(std::size_t index, std::size_t total, const RenameResult& result);
    void finishOperation(std::vector<RenameResult> results, Operation operation);
    static const char* operationName(Operation operation);
    RunSummary currentSummary() const;
    bool matchesFilter(const RenameResult& result) const;

    SDL_Window* window_ = nullptr;
    SDL_GLContext gl_context_ = nullptr;

    GameDatabase database_;
    std::string folder_path_;
    std::string database_error_;
    ImageType image_type_ = ImageType::Iso;
    NamingMode naming_mode_ = NamingMode::TitleOnly;
    Filter filter_ = Filter::All;

    mutable std::mutex state_mutex_;
    std::vector<RenameResult> results_;
    std::vector<std::string> activity_;
    std::string current_activity_ = "Select a folder to begin.";
    std::size_t progress_current_ = 0;
    std::size_t progress_total_ = 0;
    RunSummary completed_summary_{};
    Operation operation_ = Operation::None;
    bool show_failure_popup_ = false;
    bool scroll_activity_to_bottom_ = false;
    bool show_about_ = false;

    std::mutex dialog_mutex_;
    std::filesystem::path pending_folder_;
    std::filesystem::path pending_database_;
    std::filesystem::path pending_report_;

    std::thread worker_;
    std::atomic_bool running_{false};
    std::atomic_bool cancel_requested_{false};
};

} // namespace ps2br
