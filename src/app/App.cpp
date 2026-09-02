#include "app/App.h"

#include "core/Report.h"
#include "core/PathText.h"
#include "third_party_notices.h"

#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl3.h>
#include <imgui_stdlib.h>

#include <SDL3/SDL_opengl.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <system_error>

namespace ps2br {
namespace {

constexpr ImVec4 kCyan{0.12F, 0.85F, 0.92F, 1.0F};
constexpr ImVec4 kGreen{0.25F, 0.90F, 0.48F, 1.0F};
constexpr ImVec4 kRed{1.0F, 0.32F, 0.34F, 1.0F};
constexpr ImVec4 kYellow{1.0F, 0.76F, 0.24F, 1.0F};
constexpr ImVec4 kMuted{0.66F, 0.69F, 0.74F, 1.0F};

void setV4Style() {
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0F;
    style.ChildRounding = 6.0F;
    style.FrameRounding = 5.0F;
    style.PopupRounding = 7.0F;
    style.ScrollbarRounding = 6.0F;
    style.TabRounding = 5.0F;
    style.FramePadding = ImVec2(9.0F, 6.0F);
    style.ItemSpacing = ImVec2(9.0F, 7.0F);
    style.WindowPadding = ImVec2(14.0F, 12.0F);
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.055F, 0.064F, 0.080F, 1.0F);
    style.Colors[ImGuiCol_ChildBg] = ImVec4(0.075F, 0.085F, 0.105F, 1.0F);
    style.Colors[ImGuiCol_FrameBg] = ImVec4(0.105F, 0.12F, 0.145F, 1.0F);
    style.Colors[ImGuiCol_Button] = ImVec4(0.07F, 0.47F, 0.54F, 1.0F);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.08F, 0.62F, 0.70F, 1.0F);
    style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.05F, 0.36F, 0.42F, 1.0F);
    style.Colors[ImGuiCol_Header] = ImVec4(0.06F, 0.38F, 0.44F, 1.0F);
    style.Colors[ImGuiCol_CheckMark] = kCyan;
    style.Colors[ImGuiCol_SliderGrab] = kCyan;
}

} // namespace

App::App() = default;

App::~App() {
    cancel_requested_.store(true);
    if (worker_.joinable()) {
        worker_.join();
    }
    if (ImGui::GetCurrentContext() != nullptr) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL3_Shutdown();
        ImGui::DestroyContext();
    }
    if (gl_context_ != nullptr) {
        SDL_GL_DestroyContext(gl_context_);
    }
    if (window_ != nullptr) {
        SDL_DestroyWindow(window_);
    }
    SDL_Quit();
}

bool App::initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, 0);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);

    window_ = SDL_CreateWindow(
        "PS2 Batch Renamer V4 - VajskiDs",
        1100,
        720,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY);
    if (window_ == nullptr) {
        std::fprintf(stderr, "Window creation failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_SetWindowMinimumSize(window_, 900, 600);

    gl_context_ = SDL_GL_CreateContext(window_);
    if (gl_context_ == nullptr) {
        std::fprintf(stderr, "OpenGL context creation failed: %s\n", SDL_GetError());
        return false;
    }
    SDL_GL_MakeCurrent(window_, gl_context_);
    SDL_GL_SetSwapInterval(1);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    setV4Style();

    if (!ImGui_ImplSDL3_InitForOpenGL(window_, gl_context_)) {
        std::fprintf(stderr, "ImGui SDL backend initialization failed.\n");
        return false;
    }
    if (!ImGui_ImplOpenGL3_Init("#version 150")) {
        std::fprintf(stderr, "ImGui OpenGL backend initialization failed.\n");
        return false;
    }

    std::string error;
    if (!database_.loadBuiltIn(error)) {
        database_error_ = error;
    } else {
        appendActivity("Loaded built-in database: " + std::to_string(database_.recordCount()) + " records.");
    }
    return true;
}

int App::run() {
    bool done = false;
    while (!done) {
        processEvents(done);
        processDialogResults();
        reapWorker();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL3_NewFrame();
        ImGui::NewFrame();
        draw();
        ImGui::Render();

        int width = 0;
        int height = 0;
        SDL_GetWindowSizeInPixels(window_, &width, &height);
        glViewport(0, 0, width, height);
        glClearColor(0.035F, 0.041F, 0.052F, 1.0F);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        SDL_GL_SwapWindow(window_);
    }
    return 0;
}

void App::processEvents(bool& done) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);
        if (event.type == SDL_EVENT_QUIT ||
            (event.type == SDL_EVENT_WINDOW_CLOSE_REQUESTED && event.window.windowID == SDL_GetWindowID(window_))) {
            done = true;
        } else if (event.type == SDL_EVENT_DROP_FILE && event.drop.data != nullptr && !running_.load()) {
            const std::filesystem::path dropped = pathFromUtf8(event.drop.data);
            std::error_code ec;
            if (std::filesystem::is_directory(dropped, ec)) {
                invalidatePreview("Folder changed; scan again to create a new preview.");
                folder_path_ = pathToUtf8(dropped);
                appendActivity("Selected folder: " + folder_path_);
            }
        }
    }
}

void App::processDialogResults() {
    std::filesystem::path folder;
    std::filesystem::path database;
    std::filesystem::path report;
    {
        std::lock_guard lock(dialog_mutex_);
        folder.swap(pending_folder_);
        database.swap(pending_database_);
        report.swap(pending_report_);
    }
    if (running_.load()) {
        std::lock_guard lock(dialog_mutex_);
        if (!folder.empty()) pending_folder_ = std::move(folder);
        if (!database.empty()) pending_database_ = std::move(database);
        if (!report.empty()) pending_report_ = std::move(report);
        return;
    }
    if (!folder.empty()) {
        invalidatePreview("Folder changed; scan again to create a new preview.");
        folder_path_ = pathToUtf8(folder);
        appendActivity("Selected folder: " + folder_path_);
    }
    if (!database.empty()) {
        std::string error;
        if (database_.loadFile(database, error)) {
            database_error_.clear();
            invalidatePreview("Database changed; scan again to create a new preview.");
            appendActivity("Loaded database: " + pathToUtf8(database) + " (" +
                           std::to_string(database_.recordCount()) + " records).");
        } else {
            database_error_ = error;
            appendActivity("FAILED: Database was not loaded - " + error);
        }
    }
    if (!report.empty()) {
        std::vector<RenameResult> snapshot;
        {
            std::lock_guard lock(state_mutex_);
            snapshot = results_;
        }
        std::string error;
        if (saveTextReport(report, snapshot, error)) {
            appendActivity("Saved report: " + pathToUtf8(report));
        } else {
            appendActivity("FAILED: " + error);
        }
    }
}

void App::draw() {
    const ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::Begin("PS2 Batch Renamer V4", nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                 ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus);

    drawHeader();
    drawSetupPanel();
    drawProgressPanel();

    const float activity_height = 155.0F;
    drawResultsPanel();
    ImGui::Spacing();
    ImGui::SetNextItemWidth(-1.0F);
    ImGui::BeginChild("activity-region", ImVec2(0, activity_height), true);
    drawActivityPanel();
    ImGui::EndChild();
    drawFailurePopup();
    drawAboutPopup();
    ImGui::End();
}

void App::drawHeader() {
    ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
    ImGui::TextColored(kCyan, "PS2 BATCH RENAMER");
    ImGui::SameLine();
    ImGui::TextDisabled("V4  |  Native ISO + CHD");
    ImGui::SameLine(ImGui::GetContentRegionMax().x - 55.0F);
    if (ImGui::SmallButton("About")) show_about_ = true;
    ImGui::PopFont();
    ImGui::Separator();
}

void App::drawSetupPanel() {
    const bool busy = running_.load();
    ImGui::BeginDisabled(busy);
    ImGui::TextUnformatted("Game folder");
    ImGui::SetNextItemWidth(-105.0F);
    if (ImGui::InputText("##game-folder", &folder_path_)) {
        invalidatePreview("Folder changed; scan again to create a new preview.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Browse...", ImVec2(95.0F, 0))) {
        chooseFolder();
    }

    ImGui::TextUnformatted("Image type");
    ImGui::SameLine(115.0F);
    if (ImGui::RadioButton("ISO", image_type_ == ImageType::Iso) && image_type_ != ImageType::Iso) {
        image_type_ = ImageType::Iso;
        invalidatePreview("Image type changed; scan again to create a new preview.");
    }
    ImGui::SameLine();
    if (ImGui::RadioButton("CHD", image_type_ == ImageType::Chd) && image_type_ != ImageType::Chd) {
        image_type_ = ImageType::Chd;
        invalidatePreview("Image type changed; scan again to create a new preview.");
    }
    ImGui::SameLine(285.0F);
    bool include_id = naming_mode_ == NamingMode::TitleAndGameId;
    if (ImGui::Checkbox("Include game ID in filename", &include_id)) {
        naming_mode_ = include_id ? NamingMode::TitleAndGameId : NamingMode::TitleOnly;
        invalidatePreview("Naming option changed; scan again to create a new preview.");
    }

    ImGui::TextUnformatted("Database");
    ImGui::SameLine(115.0F);
    ImGui::TextColored(database_error_.empty() ? kGreen : kRed, "%s", database_.sourceName().c_str());
    ImGui::SameLine();
    ImGui::TextDisabled("%zu records, %zu duplicate IDs", database_.recordCount(), database_.duplicateIdCount());
    ImGui::SameLine();
    if (ImGui::SmallButton("Choose newer gameid.txt...")) chooseDatabase();
    if (!database_.isBuiltIn() || !database_error_.empty()) {
        ImGui::SameLine();
        if (ImGui::SmallButton("Reset to built-in")) resetDatabase();
    }
    if (!database_error_.empty()) {
        ImGui::TextColored(kRed, "Database error: %s", database_error_.c_str());
    }
    ImGui::EndDisabled();
}

void App::drawProgressPanel() {
    std::string activity;
    std::size_t current = 0;
    std::size_t total = 0;
    Operation operation = Operation::None;
    {
        std::lock_guard lock(state_mutex_);
        activity = current_activity_;
        current = progress_current_;
        total = progress_total_;
        operation = operation_;
    }
    const RunSummary summary = currentSummary();

    ImGui::Spacing();
    ImGui::TextColored(running_.load() ? kCyan : kMuted, "%s", operationName(operation));
    ImGui::SameLine();
    ImGui::TextUnformatted(activity.c_str());

    const float fraction = total == 0 ? 0.0F : static_cast<float>(current) / static_cast<float>(total);
    const std::string overlay = total == 0 ? "Waiting" : std::to_string(current) + " / " + std::to_string(total);
    ImGui::ProgressBar(fraction, ImVec2(-1.0F, 18.0F), overlay.c_str());

    ImGui::TextColored(kCyan, "Total %zu", summary.total);
    ImGui::SameLine(); ImGui::TextColored(kYellow, "Ready %zu", summary.ready);
    ImGui::SameLine(); ImGui::TextColored(kGreen, "Renamed %zu", summary.renamed);
    ImGui::SameLine(); ImGui::TextColored(kMuted, "Skipped %zu", summary.skipped);
    ImGui::SameLine(); ImGui::TextColored(summary.failed > 0 ? kRed : kMuted, "Failed %zu", summary.failed);

    if (summary.failed > 0) {
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.30F, 0.055F, 0.065F, 0.95F));
        ImGui::BeginChild("failure-banner", ImVec2(0, 38.0F), true);
        ImGui::TextColored(kRed, "Attention: %zu file%s failed.", summary.failed, summary.failed == 1 ? "" : "s");
        ImGui::SameLine();
        if (ImGui::SmallButton("Show failed files")) filter_ = Filter::Failed;
        ImGui::EndChild();
        ImGui::PopStyleColor();
    }

    const bool can_scan = !running_.load() && !folder_path_.empty() && database_error_.empty();
    ImGui::BeginDisabled(!can_scan);
    if (ImGui::Button("Scan / Preview", ImVec2(150.0F, 34.0F))) startScan();
    ImGui::EndDisabled();
    ImGui::SameLine();
    const bool can_rename = !running_.load() && summary.ready > 0;
    ImGui::BeginDisabled(!can_rename);
    if (ImGui::Button("Apply Renames", ImVec2(150.0F, 34.0F))) startRename();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(!running_.load());
    if (ImGui::Button("Cancel", ImVec2(100.0F, 34.0F))) requestCancel();
    ImGui::EndDisabled();
    ImGui::SameLine();
    ImGui::BeginDisabled(summary.total == 0 || running_.load());
    if (ImGui::Button("Copy Results", ImVec2(120.0F, 34.0F))) {
        std::vector<RenameResult> snapshot;
        { std::lock_guard lock(state_mutex_); snapshot = results_; }
        ImGui::SetClipboardText(makeTextReport(snapshot).c_str());
        appendActivity("Copied results to the clipboard.");
    }
    ImGui::SameLine();
    if (ImGui::Button("Save Report...", ImVec2(120.0F, 34.0F))) chooseReportDestination();
    ImGui::EndDisabled();
}

void App::drawResultsPanel() {
    std::vector<RenameResult> snapshot;
    {
        std::lock_guard lock(state_mutex_);
        snapshot = results_;
    }

    ImGui::Spacing();
    const std::array<std::pair<Filter, const char*>, 5> filters{{
        {Filter::All, "All"}, {Filter::Ready, "Ready"}, {Filter::Renamed, "Renamed"},
        {Filter::Failed, "Failed"}, {Filter::Skipped, "Skipped"},
    }};
    for (std::size_t index = 0; index < filters.size(); ++index) {
        if (index > 0) ImGui::SameLine();
        if (filter_ == filters[index].first) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08F, 0.60F, 0.67F, 1.0F));
        }
        if (ImGui::Button(filters[index].second)) filter_ = filters[index].first;
        if (filter_ == filters[index].first) ImGui::PopStyleColor();
    }

    std::vector<std::size_t> visible;
    visible.reserve(snapshot.size());
    for (std::size_t index = 0; index < snapshot.size(); ++index) {
        if (matchesFilter(snapshot[index])) visible.push_back(index);
    }

    const float remaining = ImGui::GetContentRegionAvail().y - 166.0F;
    if (ImGui::BeginTable("results", 5,
                          ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_Resizable |
                          ImGuiTableFlags_ScrollY | ImGuiTableFlags_SizingStretchProp,
                          ImVec2(0, std::max(150.0F, remaining)))) {
        ImGui::TableSetupScrollFreeze(0, 1);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 88.0F);
        ImGui::TableSetupColumn("Original file", ImGuiTableColumnFlags_WidthStretch, 1.5F);
        ImGui::TableSetupColumn("Game ID", ImGuiTableColumnFlags_WidthFixed, 105.0F);
        ImGui::TableSetupColumn("New filename", ImGuiTableColumnFlags_WidthStretch, 1.7F);
        ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch, 1.1F);
        ImGui::TableHeadersRow();

        ImGuiListClipper clipper;
        clipper.Begin(static_cast<int>(visible.size()));
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; ++row) {
                const auto& item = snapshot[visible[static_cast<std::size_t>(row)]];
                ImGui::TableNextRow();
                ImGui::TableNextColumn(); drawStatusBadge(item.status);
                const std::string source_filename = pathToUtf8(item.source_path.filename());
                const std::string destination_filename = item.destination_path.empty()
                    ? "-" : pathToUtf8(item.destination_path.filename());
                ImGui::TableNextColumn(); ImGui::TextUnformatted(source_filename.c_str());
                ImGui::TableNextColumn(); ImGui::TextUnformatted(item.game_id.c_str());
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(destination_filename.c_str());
                ImGui::TableNextColumn(); ImGui::TextWrapped("%s", item.detail.c_str());
            }
        }
        ImGui::EndTable();
    }
}

void App::drawActivityPanel() {
    std::vector<std::string> snapshot;
    bool scroll_to_bottom = false;
    {
        std::lock_guard lock(state_mutex_);
        snapshot = activity_;
        scroll_to_bottom = scroll_activity_to_bottom_;
        scroll_activity_to_bottom_ = false;
    }
    ImGui::TextColored(kCyan, "Live activity");
    ImGui::SameLine();
    ImGui::TextDisabled("Every scanned and renamed file appears here");
    ImGui::Separator();
    ImGuiListClipper clipper;
    clipper.Begin(static_cast<int>(snapshot.size()), ImGui::GetTextLineHeightWithSpacing());
    while (clipper.Step()) {
        for (int index = clipper.DisplayStart; index < clipper.DisplayEnd; ++index) {
            const std::string& line = snapshot[static_cast<std::size_t>(index)];
            const ImVec4 color = line.starts_with("FAILED:") ? kRed :
                                 line.starts_with("Renamed:") ? kGreen :
                                 line.starts_with("Ready:") ? kYellow : kMuted;
            ImGui::TextColored(color, "%s", line.c_str());
        }
    }
    if (scroll_to_bottom) {
        ImGui::SetScrollHereY(1.0F);
    }
}

void App::drawFailurePopup() {
    bool show_failure_popup = false;
    {
        std::lock_guard lock(state_mutex_);
        show_failure_popup = show_failure_popup_;
        show_failure_popup_ = false;
    }
    if (show_failure_popup) {
        ImGui::OpenPopup("Failed renames require attention");
    }
    ImGui::SetNextWindowSize(ImVec2(520.0F, 0), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("Failed renames require attention", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        const RunSummary summary = currentSummary();
        ImGui::TextColored(kRed, "%zu file%s could not be processed.", summary.failed, summary.failed == 1 ? "" : "s");
        ImGui::TextWrapped("Nothing was overwritten. The failed items remain in the results window with the reason shown.");
        ImGui::Spacing();
        if (ImGui::Button("View failed files", ImVec2(160.0F, 0))) {
            filter_ = Filter::Failed;
            ImGui::CloseCurrentPopup();
        }
        ImGui::SameLine();
        if (ImGui::Button("Close", ImVec2(100.0F, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void App::drawAboutPopup() {
    if (show_about_) {
        ImGui::OpenPopup("About PS2 Batch Renamer V4");
        show_about_ = false;
    }
    ImGui::SetNextWindowSize(ImVec2(650.0F, 500.0F), ImGuiCond_Appearing);
    if (ImGui::BeginPopupModal("About PS2 Batch Renamer V4", nullptr)) {
        ImGui::TextColored(kCyan, "PS2 Batch Renamer V4");
        ImGui::TextUnformatted("Native Windows and Linux edition by VajskiDs");
        ImGui::TextWrapped("The original handwritten PS2 identification method is preserved. V4 is licensed under GPL-3.0; source code is published with each release.");
        ImGui::SeparatorText("Third-party notices");
        ImGui::BeginChild("notices", ImVec2(0, -42.0F), true);
        ImGui::TextUnformatted(embedded::third_party_notices);
        ImGui::EndChild();
        if (ImGui::Button("Close", ImVec2(100.0F, 0))) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
}

void App::drawStatusBadge(ResultStatus status) const {
    const ImVec4 color = status == ResultStatus::Failed ? kRed :
                         status == ResultStatus::Renamed ? kGreen :
                         status == ResultStatus::Ready ? kYellow :
                         status == ResultStatus::Scanning ? kCyan : kMuted;
    ImGui::TextColored(color, "%s", toString(status));
}

void App::chooseFolder() {
    SDL_ShowOpenFolderDialog(folderDialogCallback, this, window_, folder_path_.empty() ? nullptr : folder_path_.c_str(), false);
}

void App::chooseDatabase() {
    static constexpr SDL_DialogFileFilter filters[]{{"Game ID database", "txt"}};
    SDL_ShowOpenFileDialog(databaseDialogCallback, this, window_, filters, 1, nullptr, false);
}

void App::chooseReportDestination() {
    static constexpr SDL_DialogFileFilter filters[]{{"Text report", "txt"}};
    SDL_ShowSaveFileDialog(reportDialogCallback, this, window_, filters, 1, "PS2-Batch-Renamer-V4-report.txt");
}

void App::resetDatabase() {
    std::string error;
    if (database_.loadBuiltIn(error)) {
        database_error_.clear();
        invalidatePreview("Database changed; scan again to create a new preview.");
        appendActivity("Reset to the built-in V4 database.");
    } else {
        database_error_ = error;
    }
}

void App::startScan() {
    reapWorker();
    std::error_code ec;
    const std::filesystem::path folder = pathFromUtf8(folder_path_);
    if (!std::filesystem::is_directory(folder, ec)) {
        appendActivity("FAILED: The selected game folder does not exist.");
        return;
    }

    {
        std::lock_guard lock(state_mutex_);
        results_.clear();
        completed_summary_ = {};
        progress_current_ = 0;
        progress_total_ = 0;
        operation_ = Operation::Scanning;
    }
    filter_ = Filter::All;
    cancel_requested_.store(false);
    running_.store(true);
    const ScanOptions options{folder, image_type_, naming_mode_};
    worker_ = std::thread([this, options] {
        Renamer renamer(database_);
        auto results = renamer.scan(
            options,
            [this](const std::string& message) { appendActivity(message); },
            [this](std::size_t index, std::size_t total, const RenameResult& result) {
                updateResult(index, total, result);
            },
            &cancel_requested_);
        finishOperation(std::move(results), Operation::Scanning);
    });
}

void App::startRename() {
    reapWorker();
    std::vector<RenameResult> work;
    {
        std::lock_guard lock(state_mutex_);
        work = results_;
        progress_current_ = 0;
        progress_total_ = work.size();
        operation_ = Operation::Renaming;
    }
    cancel_requested_.store(false);
    running_.store(true);
    worker_ = std::thread([this, work = std::move(work)]() mutable {
        Renamer renamer(database_);
        renamer.apply(
            work,
            [this](const std::string& message) { appendActivity(message); },
            [this](std::size_t index, std::size_t total, const RenameResult& result) {
                updateResult(index, total, result);
            },
            &cancel_requested_);
        finishOperation(std::move(work), Operation::Renaming);
    });
}

void App::requestCancel() {
    cancel_requested_.store(true);
    appendActivity("Cancellation requested; the current file will finish safely.");
}

void App::reapWorker() {
    if (!running_.load() && worker_.joinable()) {
        worker_.join();
    }
}

void App::invalidatePreview(std::string reason) {
    bool had_results = false;
    {
        std::lock_guard lock(state_mutex_);
        had_results = !results_.empty();
        results_.clear();
        completed_summary_ = {};
        progress_current_ = 0;
        progress_total_ = 0;
        operation_ = Operation::None;
    }
    if (had_results) appendActivity(std::move(reason));
}

void App::appendActivity(std::string message) {
    std::lock_guard lock(state_mutex_);
    current_activity_ = message;
    activity_.push_back(std::move(message));
    if (activity_.size() > 50000) {
        activity_.erase(activity_.begin(), activity_.begin() + 10000);
    }
    scroll_activity_to_bottom_ = true;
}

void App::updateResult(std::size_t index, std::size_t total, const RenameResult& result) {
    std::lock_guard lock(state_mutex_);
    if (results_.size() <= index) results_.resize(index + 1);
    if (index < results_.size()) results_[index] = result;
    progress_current_ = std::min(index + 1, total);
    progress_total_ = total;
}

void App::finishOperation(std::vector<RenameResult> results, Operation operation) {
    std::lock_guard lock(state_mutex_);
    const bool cancelled = cancel_requested_.load();
    results_ = std::move(results);
    completed_summary_ = Renamer::summarize(results_);
    progress_total_ = results_.size();
    progress_current_ = results_.size();
    operation_ = Operation::None;
    if (cancelled) {
        current_activity_ = operation == Operation::Scanning
            ? "Scan cancelled. The results processed so far are still available."
            : "Rename operation cancelled. Unprocessed ready items are still available.";
    } else if (completed_summary_.failed > 0) {
        show_failure_popup_ = true;
        current_activity_ = std::to_string(completed_summary_.failed) + " file(s) failed. Review required.";
    } else if (operation == Operation::Scanning) {
        current_activity_ = "Scan complete. Review the proposed names, then apply renames.";
    } else {
        current_activity_ = "Rename operation complete.";
    }
    activity_.push_back(current_activity_);
    scroll_activity_to_bottom_ = true;
    running_.store(false);
}

RunSummary App::currentSummary() const {
    std::lock_guard lock(state_mutex_);
    return Renamer::summarize(results_);
}

const char* App::operationName(Operation operation) {
    switch (operation) {
    case Operation::Scanning: return "Scanning";
    case Operation::Renaming: return "Renaming";
    default: return "Idle";
    }
}

bool App::matchesFilter(const RenameResult& result) const {
    switch (filter_) {
    case Filter::All: return true;
    case Filter::Ready: return result.status == ResultStatus::Ready;
    case Filter::Renamed: return result.status == ResultStatus::Renamed;
    case Filter::Failed: return result.status == ResultStatus::Failed;
    case Filter::Skipped: return result.status == ResultStatus::Skipped;
    }
    return true;
}

void SDLCALL App::folderDialogCallback(void* userdata, const char* const* files, int) {
    if (files == nullptr || files[0] == nullptr) return;
    auto* app = static_cast<App*>(userdata);
    std::lock_guard lock(app->dialog_mutex_);
    app->pending_folder_ = pathFromUtf8(files[0]);
}

void SDLCALL App::databaseDialogCallback(void* userdata, const char* const* files, int) {
    if (files == nullptr || files[0] == nullptr) return;
    auto* app = static_cast<App*>(userdata);
    std::lock_guard lock(app->dialog_mutex_);
    app->pending_database_ = pathFromUtf8(files[0]);
}

void SDLCALL App::reportDialogCallback(void* userdata, const char* const* files, int) {
    if (files == nullptr || files[0] == nullptr) return;
    auto* app = static_cast<App*>(userdata);
    std::lock_guard lock(app->dialog_mutex_);
    app->pending_report_ = pathFromUtf8(files[0]);
}

} // namespace ps2br
