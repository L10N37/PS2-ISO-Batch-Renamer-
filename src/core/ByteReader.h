#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>

namespace ps2br {

class ByteReader {
public:
    virtual ~ByteReader() = default;
    virtual bool read(std::uint64_t offset, std::span<std::byte> destination, std::string& error) = 0;
};

} // namespace ps2br
