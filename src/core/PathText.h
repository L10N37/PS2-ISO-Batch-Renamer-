#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace ps2br {

std::filesystem::path pathFromUtf8(std::string_view text);
std::string pathToUtf8(const std::filesystem::path& path);

} // namespace ps2br
