#pragma once

#include "core/ByteReader.h"

#include <filesystem>
#include <fstream>

namespace ps2br {

class IsoReader final : public ByteReader {
public:
    explicit IsoReader(const std::filesystem::path& path);

    bool isOpen() const;
    const std::string& openError() const;
    bool read(std::uint64_t offset, std::span<std::byte> destination, std::string& error) override;

private:
    std::ifstream file_;
    std::string open_error_;
};

} // namespace ps2br
