#include "core/Renamer.h"

#include "core/Identifier.h"
#include "core/IsoReader.h"
#include "core/PathText.h"
#if PS2BR_ENABLE_CHD
#include "core/ChdReader.h"
#endif

#include <algorithm>
#include <array>
#include <cctype>
#include <memory>
#include <system_error>

namespace ps2br {
namespace {

std::string lowerAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char value) {
        return static_cast<char>(std::tolower(value));
    });
    return text;
}

bool isCancelled(const std::atomic_bool* cancel) {
    return cancel != nullptr && cancel->load();
}

std::string filenameFor(const DatabaseRecord& record, NamingMode mode, ImageType type) {
    std::string filename = record.title;
    if (mode == NamingMode::TitleAndGameId) {
        filename += " (" + record.game_id + ")";
    }
    filename += (type == ImageType::Iso ? ".iso" : ".chd");
    return filename;
}

std::string portableFilenameError(std::string_view filename) {
    constexpr std::string_view forbidden = "<>:\"/\\|?*";
    for (unsigned char value : filename) {
        if (value < 32 || forbidden.find(static_cast<char>(value)) != std::string_view::npos) {
            return "Database title contains a character that is not valid in a Windows filename";
        }
    }
    if (filename.empty() || filename.back() == ' ' || filename.back() == '.') {
        return "Database title ends with a character that is not valid in a Windows filename";
    }

    const std::size_t dot = filename.find('.');
    const std::string stem = lowerAscii(std::string(filename.substr(0, dot)));
    constexpr std::array<std::string_view, 4> reserved{{"con", "prn", "aux", "nul"}};
    if (std::find(reserved.begin(), reserved.end(), stem) != reserved.end() ||
        (stem.size() == 4 && (stem.starts_with("com") || stem.starts_with("lpt")) &&
         stem[3] >= '1' && stem[3] <= '9')) {
        return "Database title produces a filename reserved by Windows";
    }
    return {};
}

std::unique_ptr<ByteReader> openReader(const std::filesystem::path& path, ImageType type, std::string& error) {
    if (type == ImageType::Iso) {
        auto reader = std::make_unique<IsoReader>(path);
        if (!reader->isOpen()) {
            error = reader->openError();
            return nullptr;
        }
        return reader;
    }
#if PS2BR_ENABLE_CHD
    auto reader = std::make_unique<ChdReader>(path);
    if (!reader->isOpen()) {
        error = reader->openError();
        return nullptr;
    }
    return reader;
#else
    error = "This build does not include CHD support";
    return nullptr;
#endif
}

} // namespace

const char* toString(ImageType type) {
    return type == ImageType::Iso ? "ISO" : "CHD";
}

const char* toString(ResultStatus status) {
    switch (status) {
    case ResultStatus::Pending: return "Pending";
    case ResultStatus::Scanning: return "Scanning";
    case ResultStatus::Ready: return "Ready";
    case ResultStatus::Renamed: return "Renamed";
    case ResultStatus::Skipped: return "Skipped";
    case ResultStatus::Failed: return "Failed";
    }
    return "Unknown";
}

Renamer::Renamer(const GameDatabase& database) : database_(database) {}

std::vector<RenameResult> Renamer::scan(
    const ScanOptions& options,
    const ActivityCallback& activity,
    const ResultCallback& result_update,
    const std::atomic_bool* cancel) const {

    std::vector<std::filesystem::path> paths;
    std::error_code ec;
    const std::string wanted_extension = options.image_type == ImageType::Iso ? ".iso" : ".chd";

    for (std::filesystem::directory_iterator it(options.folder, ec), end; !ec && it != end; it.increment(ec)) {
        if (it->is_regular_file(ec) && lowerAscii(pathToUtf8(it->path().extension())) == wanted_extension) {
            paths.push_back(it->path());
        }
    }
    if (ec) {
        RenameResult failure;
        failure.source_path = options.folder;
        failure.image_type = options.image_type;
        failure.status = ResultStatus::Failed;
        failure.detail = "Could not enumerate the selected folder: " + ec.message();
        if (activity) activity("FAILED: " + failure.detail);
        if (result_update) result_update(0, 1, failure);
        return {std::move(failure)};
    }

    std::sort(paths.begin(), paths.end(), [](const auto& left, const auto& right) {
        return lowerAscii(pathToUtf8(left.filename())) < lowerAscii(pathToUtf8(right.filename()));
    });

    std::vector<RenameResult> results(paths.size());
    if (activity) {
        activity("Found " + std::to_string(paths.size()) + " " + toString(options.image_type) + " files.");
    }

    for (std::size_t index = 0; index < paths.size(); ++index) {
        if (isCancelled(cancel)) {
            if (activity) activity("Scan cancelled.");
            results.resize(index);
            break;
        }

        auto& item = results[index];
        item.source_path = paths[index];
        item.image_type = options.image_type;
        item.status = ResultStatus::Scanning;
        item.detail = "Reading disc metadata";
        if (result_update) result_update(index, results.size(), item);
        if (activity) {
            activity("Scanning " + std::to_string(index + 1) + "/" + std::to_string(paths.size()) + ": " + pathToUtf8(paths[index].filename()));
        }

        std::string open_error;
        auto reader = openReader(paths[index], options.image_type, open_error);
        if (!reader) {
            item.status = ResultStatus::Failed;
            item.detail = open_error;
            if (activity) activity("FAILED: " + pathToUtf8(paths[index].filename()) + " - " + item.detail);
            if (result_update) result_update(index, results.size(), item);
            continue;
        }

        const Identification identification = identifyGame(*reader);
        if (!identification.success) {
            item.status = ResultStatus::Failed;
            item.detail = identification.error;
            if (activity) activity("FAILED: " + pathToUtf8(paths[index].filename()) + " - " + item.detail);
            if (result_update) result_update(index, results.size(), item);
            continue;
        }
        item.game_id = identification.game_id;

        const DatabaseRecord* record = database_.findFirst(item.game_id);
        if (record == nullptr) {
            item.status = ResultStatus::Failed;
            item.detail = "Game ID " + item.game_id + " is not present in the database";
            if (activity) activity("FAILED: " + pathToUtf8(paths[index].filename()) + " - " + item.detail);
            if (result_update) result_update(index, results.size(), item);
            continue;
        }

        item.database_title = record->title;
        const std::string destination_filename = filenameFor(*record, options.naming_mode, options.image_type);
        item.destination_path = options.folder / pathFromUtf8(destination_filename);
        if (const std::string filename_error = portableFilenameError(destination_filename); !filename_error.empty()) {
            item.status = ResultStatus::Failed;
            item.detail = filename_error;
            if (activity) activity("FAILED: " + pathToUtf8(paths[index].filename()) + " - " + item.detail);
            if (result_update) result_update(index, results.size(), item);
            continue;
        }
        if (item.source_path.filename() == item.destination_path.filename()) {
            item.status = ResultStatus::Skipped;
            item.detail = "Already has the requested filename";
            if (activity) activity("Skipped: " + pathToUtf8(item.source_path.filename()) + " is already named correctly.");
        } else {
            item.status = ResultStatus::Ready;
            item.detail = "Ready to rename";
            if (activity) {
                activity("Ready: " + pathToUtf8(item.source_path.filename()) + " -> " + pathToUtf8(item.destination_path.filename()));
            }
        }
        if (result_update) result_update(index, results.size(), item);
    }

    return results;
}

RunSummary Renamer::apply(
    std::vector<RenameResult>& results,
    const ActivityCallback& activity,
    const ResultCallback& result_update,
    const std::atomic_bool* cancel) const {

    for (std::size_t index = 0; index < results.size(); ++index) {
        auto& item = results[index];
        if (isCancelled(cancel)) {
            if (activity) activity("Rename operation cancelled.");
            break;
        }
        if (item.status != ResultStatus::Ready) {
            continue;
        }

        if (activity) {
            activity("Renaming " + std::to_string(index + 1) + "/" + std::to_string(results.size()) + ": " +
                     pathToUtf8(item.source_path.filename()));
        }

        std::error_code ec;
        if (std::filesystem::exists(item.destination_path, ec)) {
            item.status = ResultStatus::Failed;
            item.detail = "Destination already exists; nothing was overwritten";
        } else if (ec) {
            item.status = ResultStatus::Failed;
            item.detail = "Could not check the destination: " + ec.message();
        } else {
            std::filesystem::rename(item.source_path, item.destination_path, ec);
            if (ec) {
                item.status = ResultStatus::Failed;
                item.detail = "Rename failed: " + ec.message();
            } else {
                item.status = ResultStatus::Renamed;
                item.detail = "Renamed successfully";
            }
        }

        if (activity) {
            const std::string prefix = item.status == ResultStatus::Renamed ? "Renamed: " : "FAILED: ";
            activity(prefix + pathToUtf8(item.source_path.filename()) +
                     (item.status == ResultStatus::Renamed
                          ? " -> " + pathToUtf8(item.destination_path.filename())
                          : " - " + item.detail));
        }
        if (result_update) result_update(index, results.size(), item);
    }

    return summarize(results);
}

RunSummary Renamer::summarize(const std::vector<RenameResult>& results) {
    RunSummary summary;
    summary.total = results.size();
    for (const auto& item : results) {
        switch (item.status) {
        case ResultStatus::Ready: ++summary.ready; break;
        case ResultStatus::Renamed: ++summary.renamed; break;
        case ResultStatus::Skipped: ++summary.skipped; break;
        case ResultStatus::Failed: ++summary.failed; break;
        default: break;
        }
    }
    return summary;
}

} // namespace ps2br
