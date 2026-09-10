#pragma once

#include <cstddef>
#include <string>
#include <string_view>

namespace ps2br {

inline constexpr std::string_view kOnlineDatabaseUrl =
    "https://raw.githubusercontent.com/L10N37/PS2-ISO-Batch-Renamer-/main/gameid.txt";
inline constexpr std::size_t kOnlineDatabaseMaximumBytes = 4U * 1024U * 1024U;
inline constexpr std::size_t kOnlineDatabaseMinimumRecords = 10000;

struct OnlineDatabaseResult {
    bool success = false;
    std::string text;
    std::string error;
};

bool validateOnlineDatabase(std::string_view text, std::string& error);
OnlineDatabaseResult downloadOnlineDatabase();

} // namespace ps2br
