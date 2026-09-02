#include "core/ChdReader.h"

extern "C" {
#include <libchdr/cdrom.h>
#include <libchdr/chd.h>
}

#include <algorithm>
#include <array>
#include <cctype>
#include <cstring>

namespace ps2br {
namespace {

constexpr std::uint64_t kCookedSectorSize = 2048;
constexpr std::uint64_t kChdCdFrameSize = CD_FRAME_SIZE;

std::string upperAscii(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char value) {
        return static_cast<char>(std::toupper(value));
    });
    return text;
}

bool dataOffsetForTrack(std::string type, std::uint32_t& offset) {
    type = upperAscii(std::move(type));
    if (type == "MODE1" || type == "MODE1/2048") {
        offset = 0;
        return true;
    }
    if (type == "MODE1_RAW" || type == "MODE1/2352") {
        offset = 16;
        return true;
    }
    if (type == "MODE2_FORM1" || type == "MODE2/2048") {
        offset = 0;
        return true;
    }
    if (type == "MODE2_RAW" || type == "MODE2/2352") {
        offset = 24;
        return true;
    }
    if (type == "MODE2" || type == "MODE2_FORM_MIX" || type == "MODE2/2336") {
        offset = 8;
        return true;
    }
    return false;
}

} // namespace

ChdReader::ChdReader(const std::filesystem::path& path) {
#if defined(_WIN32)
    source_file_ = _wfopen(path.c_str(), L"rb");
#else
    source_file_ = std::fopen(path.c_str(), "rb");
#endif
    if (source_file_ == nullptr) {
        open_error_ = "Could not open the CHD for reading";
        return;
    }

    const chd_error open_result = chd_open_file(source_file_, CHD_OPEN_READ, nullptr, &chd_);
    if (open_result != CHDERR_NONE) {
        open_error_ = std::string("Could not open the CHD: ") + chd_error_string(open_result);
        std::fclose(source_file_);
        source_file_ = nullptr;
        return;
    }

    if (!configureLayout(open_error_)) {
        chd_close(chd_);
        chd_ = nullptr;
        std::fclose(source_file_);
        source_file_ = nullptr;
    }
}

ChdReader::~ChdReader() {
    if (chd_ != nullptr) {
        chd_close(chd_);
    }
    if (source_file_ != nullptr) {
        std::fclose(source_file_);
    }
}

bool ChdReader::isOpen() const {
    return chd_ != nullptr;
}

const std::string& ChdReader::openError() const {
    return open_error_;
}

bool ChdReader::configureLayout(std::string& error) {
    const chd_header* header = chd_get_header(chd_);
    if (header == nullptr || header->hunkbytes == 0) {
        error = "CHD has an invalid header";
        return false;
    }

    hunk_bytes_ = header->hunkbytes;
    logical_bytes_ = header->logicalbytes;
    hunk_buffer_.resize(hunk_bytes_);

    std::array<char, 256> metadata{};
    const bool has_cd_metadata =
        chd_get_metadata(chd_, CDROM_TRACK_METADATA_TAG, 0, metadata.data(), metadata.size(), nullptr, nullptr, nullptr) == CHDERR_NONE ||
        chd_get_metadata(chd_, CDROM_TRACK_METADATA2_TAG, 0, metadata.data(), metadata.size(), nullptr, nullptr, nullptr) == CHDERR_NONE ||
        chd_get_metadata(chd_, GDROM_TRACK_METADATA_TAG, 0, metadata.data(), metadata.size(), nullptr, nullptr, nullptr) == CHDERR_NONE;

    if (has_cd_metadata) {
        cd_layout_ = true;
        return configureCdLayout(error);
    }

    cd_layout_ = false;
    return true;
}

bool ChdReader::configureCdLayout(std::string& error) {
    if (hunk_bytes_ % kChdCdFrameSize != 0) {
        error = "CD CHD uses an unsupported hunk layout";
        return false;
    }

    std::uint64_t chd_frame_offset = 0;
    bool found_data_track = false;
    std::uint64_t data_track_pregap = 0;

    for (std::uint32_t index = 0; index < CD_MAX_TRACKS; ++index) {
        std::array<char, 256> metadata{};
        int track_number = 0;
        int frames = 0;
        int pregap = 0;
        int postgap = 0;
        int padframes = 0;
        std::array<char, 32> type{};
        std::array<char, 32> subtype{};
        std::array<char, 32> pregap_type{};
        std::array<char, 32> pregap_subtype{};
        bool parsed = false;

        chd_error metadata_result = chd_get_metadata(
            chd_, CDROM_TRACK_METADATA_TAG, index, metadata.data(), metadata.size(), nullptr, nullptr, nullptr);
        if (metadata_result == CHDERR_NONE) {
            parsed = std::sscanf(metadata.data(), CDROM_TRACK_METADATA_FORMAT,
                                 &track_number, type.data(), subtype.data(), &frames) == 4;
        } else {
            metadata.fill(0);
            metadata_result = chd_get_metadata(
                chd_, CDROM_TRACK_METADATA2_TAG, index, metadata.data(), metadata.size(), nullptr, nullptr, nullptr);
            if (metadata_result == CHDERR_NONE) {
                parsed = std::sscanf(metadata.data(), CDROM_TRACK_METADATA2_FORMAT,
                                     &track_number, type.data(), subtype.data(), &frames,
                                     &pregap, pregap_type.data(), pregap_subtype.data(), &postgap) == 8;
            } else {
                metadata.fill(0);
                metadata_result = chd_get_metadata(
                    chd_, GDROM_TRACK_METADATA_TAG, index, metadata.data(), metadata.size(), nullptr, nullptr, nullptr);
                if (metadata_result == CHDERR_NONE) {
                    parsed = std::sscanf(metadata.data(), GDROM_TRACK_METADATA_FORMAT,
                                         &track_number, type.data(), subtype.data(), &frames, &padframes,
                                         &pregap, pregap_type.data(), pregap_subtype.data(), &postgap) == 9;
                }
            }
        }

        if (metadata_result != CHDERR_NONE) {
            break;
        }
        if (!parsed || frames <= 0) {
            error = "CHD contains invalid CD track metadata";
            return false;
        }

        std::uint32_t data_offset = 0;
        if (!found_data_track && dataOffsetForTrack(type.data(), data_offset)) {
            data_track_.chd_frame_offset = chd_frame_offset;
            data_track_.frames = static_cast<std::uint64_t>(frames);
            data_track_.data_offset = data_offset;
            data_track_pregap = static_cast<std::uint64_t>(std::max(pregap, 0));
            found_data_track = true;
        }

        const std::uint64_t padded_frames =
            (static_cast<std::uint64_t>(frames) + CD_TRACK_PADDING - 1) / CD_TRACK_PADDING * CD_TRACK_PADDING;
        chd_frame_offset += padded_frames + static_cast<std::uint64_t>(std::max(padframes, 0));
    }

    if (!found_data_track) {
        error = "CHD does not contain a supported data track";
        return false;
    }

    const DataTrack metadata_layout = data_track_;
    const bool stored_pregap = data_track_pregap > 0;
    const std::array<std::uint64_t, 2> starts{{stored_pregap ? data_track_pregap : 0, 0}};
    const std::array<std::uint32_t, 5> offsets{{
        metadata_layout.data_offset, 0, 16, 24, 8,
    }};
    std::array<std::byte, kCookedSectorSize> sector{};
    for (const std::uint64_t start : starts) {
        if (start >= metadata_layout.frames || metadata_layout.frames - start <= 16) continue;
        for (std::size_t offset_index = 0; offset_index < offsets.size(); ++offset_index) {
            if (std::find(offsets.begin(), offsets.begin() + static_cast<std::ptrdiff_t>(offset_index),
                          offsets[offset_index]) != offsets.begin() + static_cast<std::ptrdiff_t>(offset_index)) {
                continue;
            }
            data_track_.chd_frame_offset = metadata_layout.chd_frame_offset + start;
            data_track_.frames = metadata_layout.frames - start;
            data_track_.data_offset = offsets[offset_index];
            std::string read_error;
            if (readCdSector(16, sector, read_error) &&
                std::memcmp(sector.data() + 1, "CD001", 5) == 0) {
                return true;
            }
        }
    }

    error = "CHD data track does not contain a readable ISO 9660 volume descriptor";
    return false;
}

bool ChdReader::readHunk(std::uint32_t hunk, std::string& error) {
    if (cached_hunk_ == hunk) {
        return true;
    }
    const chd_error read_result = chd_read(chd_, hunk, hunk_buffer_.data());
    if (read_result != CHDERR_NONE) {
        error = std::string("Could not decompress CHD hunk: ") + chd_error_string(read_result);
        return false;
    }
    cached_hunk_ = hunk;
    return true;
}

bool ChdReader::readRaw(std::uint64_t offset, std::span<std::byte> destination, std::string& error) {
    if (offset > logical_bytes_ || destination.size() > logical_bytes_ - offset) {
        error = "CHD ended before the required bytes could be read";
        return false;
    }

    while (!destination.empty()) {
        const std::uint64_t hunk64 = offset / hunk_bytes_;
        if (hunk64 > std::numeric_limits<std::uint32_t>::max()) {
            error = "CHD hunk index is too large";
            return false;
        }
        if (!readHunk(static_cast<std::uint32_t>(hunk64), error)) {
            return false;
        }
        const std::size_t within_hunk = static_cast<std::size_t>(offset % hunk_bytes_);
        const std::size_t amount = std::min(destination.size(), hunk_buffer_.size() - within_hunk);
        std::copy_n(hunk_buffer_.begin() + static_cast<std::ptrdiff_t>(within_hunk), amount, destination.begin());
        destination = destination.subspan(amount);
        offset += amount;
    }
    return true;
}

bool ChdReader::readCdSector(std::uint64_t sector, std::span<std::byte, 2048> destination, std::string& error) {
    if (sector >= data_track_.frames) {
        error = "CD data track ended before the required sector";
        return false;
    }

    const std::uint64_t frame = data_track_.chd_frame_offset + sector;
    const std::uint64_t chd_byte_offset = frame * kChdCdFrameSize + data_track_.data_offset;
    const std::uint64_t hunk64 = chd_byte_offset / hunk_bytes_;
    if (hunk64 > std::numeric_limits<std::uint32_t>::max()) {
        error = "CHD hunk index is too large";
        return false;
    }
    if (!readHunk(static_cast<std::uint32_t>(hunk64), error)) {
        return false;
    }

    const std::size_t within_hunk = static_cast<std::size_t>(chd_byte_offset % hunk_bytes_);
    if (within_hunk + destination.size() > hunk_buffer_.size()) {
        error = "CHD sector crosses an unsupported hunk boundary";
        return false;
    }
    std::copy_n(hunk_buffer_.begin() + static_cast<std::ptrdiff_t>(within_hunk), destination.size(), destination.begin());
    return true;
}

bool ChdReader::readCd(std::uint64_t offset, std::span<std::byte> destination, std::string& error) {
    std::array<std::byte, kCookedSectorSize> sector_buffer{};
    while (!destination.empty()) {
        const std::uint64_t sector = offset / kCookedSectorSize;
        const std::size_t within_sector = static_cast<std::size_t>(offset % kCookedSectorSize);
        if (!readCdSector(sector, sector_buffer, error)) {
            return false;
        }
        const std::size_t amount = std::min(destination.size(), sector_buffer.size() - within_sector);
        std::copy_n(sector_buffer.begin() + static_cast<std::ptrdiff_t>(within_sector), amount, destination.begin());
        destination = destination.subspan(amount);
        offset += amount;
    }
    return true;
}

bool ChdReader::read(std::uint64_t offset, std::span<std::byte> destination, std::string& error) {
    if (chd_ == nullptr) {
        error = open_error_.empty() ? "CHD is not open" : open_error_;
        return false;
    }
    return cd_layout_ ? readCd(offset, destination, error) : readRaw(offset, destination, error);
}

} // namespace ps2br
