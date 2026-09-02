#include "core/Identifier.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>

namespace ps2br {
namespace {

constexpr std::uint64_t kRootDirectorySectorOffset = 0x80A4;
constexpr std::uint64_t kSectorSize = 2048;
constexpr std::size_t kSystemCnfExtentDistance = 31;

std::uint8_t byteValue(std::byte value) {
    return std::to_integer<std::uint8_t>(value);
}

} // namespace

Identification identifyGame(ByteReader& reader) {
    Identification result;
    std::string error;

    std::array<std::byte, 2> root_location{};
    if (!reader.read(kRootDirectorySectorOffset, root_location, error)) {
        result.error = "Root-directory location: " + error;
        return result;
    }

    const std::uint32_t root_sector =
        (static_cast<std::uint32_t>(byteValue(root_location[0])) << 8U) |
        static_cast<std::uint32_t>(byteValue(root_location[1]));
    result.root_directory_offset = static_cast<std::uint64_t>(root_sector) * kSectorSize;

    std::array<std::byte, kSectorSize> root_directory{};
    if (!reader.read(result.root_directory_offset, root_directory, error)) {
        result.error = "Root directory: " + error;
        return result;
    }

    constexpr std::array<std::byte, 6> system_cnf_name{
        std::byte{0x45}, std::byte{0x4D}, std::byte{0x2E},
        std::byte{0x43}, std::byte{0x4E}, std::byte{0x46},
    };
    const auto cnf = std::search(root_directory.begin(), root_directory.end(),
                                 system_cnf_name.begin(), system_cnf_name.end());
    if (cnf == root_directory.end()) {
        result.error = "SYSTEM.CNF directory record was not found";
        return result;
    }

    const std::size_t cnf_index = static_cast<std::size_t>(std::distance(root_directory.begin(), cnf));
    if (cnf_index < kSystemCnfExtentDistance) {
        result.error = "SYSTEM.CNF directory record is malformed";
        return result;
    }

    const std::size_t extent = cnf_index - kSystemCnfExtentDistance;
    const std::uint32_t system_sector =
        (static_cast<std::uint32_t>(byteValue(root_directory[extent])) << 24U) |
        (static_cast<std::uint32_t>(byteValue(root_directory[extent + 1])) << 16U) |
        (static_cast<std::uint32_t>(byteValue(root_directory[extent + 2])) << 8U) |
        static_cast<std::uint32_t>(byteValue(root_directory[extent + 3]));
    result.system_cnf_offset = static_cast<std::uint64_t>(system_sector) * kSectorSize;

    std::array<std::byte, 64> system_cnf{};
    if (!reader.read(result.system_cnf_offset, system_cnf, error)) {
        result.error = "SYSTEM.CNF: " + error;
        return result;
    }

    constexpr std::array<std::byte, 3> id_precursor{
        std::byte{0x30}, std::byte{0x3A}, std::byte{0x5C},
    };
    const auto precursor = std::search(system_cnf.begin(), system_cnf.end(),
                                       id_precursor.begin(), id_precursor.end());
    if (precursor == system_cnf.end()) {
        result.error = "Game-ID precursor 0:\\ was not found in SYSTEM.CNF";
        return result;
    }

    const auto game_id_begin = precursor + static_cast<std::ptrdiff_t>(id_precursor.size());
    if (std::distance(game_id_begin, system_cnf.end()) < 11) {
        result.error = "SYSTEM.CNF ended before the complete game ID";
        return result;
    }

    result.game_id.reserve(11);
    for (auto it = game_id_begin; it != game_id_begin + 11; ++it) {
        result.game_id.push_back(static_cast<char>(byteValue(*it)));
    }
    result.success = true;
    return result;
}

} // namespace ps2br
