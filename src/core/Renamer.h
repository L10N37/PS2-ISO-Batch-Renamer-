#pragma once

#include "core/Database.h"
#include "core/Types.h"

#include <atomic>
#include <filesystem>
#include <functional>
#include <vector>

namespace ps2br {

using ActivityCallback = std::function<void(const std::string&)>;
using ResultCallback = std::function<void(std::size_t, std::size_t, const RenameResult&)>;

struct ScanOptions {
    std::filesystem::path folder;
    ImageType image_type = ImageType::Iso;
    NamingMode naming_mode = NamingMode::TitleOnly;
};

class Renamer {
public:
    explicit Renamer(const GameDatabase& database);

    std::vector<RenameResult> scan(
        const ScanOptions& options,
        const ActivityCallback& activity,
        const ResultCallback& result_update,
        const std::atomic_bool* cancel = nullptr) const;

    RunSummary apply(
        std::vector<RenameResult>& results,
        const ActivityCallback& activity,
        const ResultCallback& result_update,
        const std::atomic_bool* cancel = nullptr) const;

    static RunSummary summarize(const std::vector<RenameResult>& results);

private:
    const GameDatabase& database_;
};

} // namespace ps2br
