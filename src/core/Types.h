#pragma once

#include <filesystem>
#include <string>

namespace ps2br {

enum class ImageType {
    Iso,
    Chd,
};

enum class NamingMode {
    TitleOnly,
    TitleAndGameId,
};

enum class ResultStatus {
    Pending,
    Scanning,
    Ready,
    Renamed,
    Skipped,
    Failed,
};

struct Identification {
    bool success = false;
    std::string game_id;
    std::string error;
    std::uint64_t root_directory_offset = 0;
    std::uint64_t system_cnf_offset = 0;
};

struct RenameResult {
    std::filesystem::path source_path;
    std::filesystem::path destination_path;
    ImageType image_type = ImageType::Iso;
    ResultStatus status = ResultStatus::Pending;
    std::string game_id;
    std::string database_title;
    std::string detail;
};

struct RunSummary {
    std::size_t total = 0;
    std::size_t ready = 0;
    std::size_t renamed = 0;
    std::size_t skipped = 0;
    std::size_t failed = 0;
};

const char* toString(ImageType type);
const char* toString(ResultStatus status);

} // namespace ps2br
