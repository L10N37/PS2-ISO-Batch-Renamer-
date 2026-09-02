#include "core/PathText.h"

namespace ps2br {

std::filesystem::path pathFromUtf8(std::string_view text) {
    const std::u8string utf8(reinterpret_cast<const char8_t*>(text.data()), text.size());
    return std::filesystem::path(utf8);
}

std::string pathToUtf8(const std::filesystem::path& path) {
    const std::u8string text = path.generic_u8string();
    return {reinterpret_cast<const char*>(text.data()), text.size()};
}

} // namespace ps2br
