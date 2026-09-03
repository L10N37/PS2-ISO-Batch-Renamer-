#pragma once

#include "core/Types.h"

#include <filesystem>
#include <span>
#include <string>

namespace ps2br {

std::string makeTextReport(std::span<const RenameResult> results);
bool saveTextReport(const std::filesystem::path& path, std::span<const RenameResult> results, std::string& error);

} // namespace ps2br
