#include "core/Report.h"

#include "core/PathText.h"
#include "core/Renamer.h"

#include <fstream>
#include <sstream>
#include <vector>

namespace ps2br {

std::string makeTextReport(std::span<const RenameResult> results) {
    std::ostringstream output;
    const RunSummary summary = Renamer::summarize(std::vector<RenameResult>(results.begin(), results.end()));
    output << "PS2 Batch Renamer V4 report\n"
           << "Total: " << summary.total
           << " | Renamed: " << summary.renamed
           << " | Ready: " << summary.ready
           << " | Skipped: " << summary.skipped
           << " | Failed: " << summary.failed << "\n\n";

    for (const auto& item : results) {
        output << '[' << toString(item.status) << "] " << pathToUtf8(item.source_path.filename());
        if (!item.game_id.empty()) output << " | " << item.game_id;
        if (!item.destination_path.empty()) output << " | " << pathToUtf8(item.destination_path.filename());
        if (!item.detail.empty()) output << " | " << item.detail;
        output << '\n';
    }
    return output.str();
}

bool saveTextReport(const std::filesystem::path& path, std::span<const RenameResult> results, std::string& error) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        error = "Could not create the report file";
        return false;
    }
    file << makeTextReport(results);
    if (!file) {
        error = "Could not finish writing the report file";
        return false;
    }
    return true;
}

} // namespace ps2br
