#include "core/IsoReader.h"

#include <limits>

namespace ps2br {

IsoReader::IsoReader(const std::filesystem::path& path)
    : file_(path, std::ios::binary) {
    if (!file_) {
        open_error_ = "Could not open the image for reading";
    }
}

bool IsoReader::isOpen() const {
    return file_.is_open();
}

const std::string& IsoReader::openError() const {
    return open_error_;
}

bool IsoReader::read(std::uint64_t offset, std::span<std::byte> destination, std::string& error) {
    if (!file_) {
        error = open_error_.empty() ? "Image is not open" : open_error_;
        return false;
    }
    if (offset > static_cast<std::uint64_t>(std::numeric_limits<std::streamoff>::max())) {
        error = "Requested image offset is too large";
        return false;
    }

    file_.clear();
    file_.seekg(static_cast<std::streamoff>(offset), std::ios::beg);
    if (!file_) {
        error = "Could not seek to the requested image offset";
        return false;
    }

    file_.read(reinterpret_cast<char*>(destination.data()), static_cast<std::streamsize>(destination.size()));
    if (file_.gcount() != static_cast<std::streamsize>(destination.size())) {
        error = "Image ended before the required bytes could be read";
        return false;
    }
    return true;
}

} // namespace ps2br
