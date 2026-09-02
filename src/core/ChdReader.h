#pragma once

#include "core/ByteReader.h"

#include <cstdio>
#include <filesystem>
#include <limits>
#include <string>
#include <vector>

struct _chd_file;
typedef struct _chd_file chd_file;

namespace ps2br {

class ChdReader final : public ByteReader {
public:
    explicit ChdReader(const std::filesystem::path& path);
    ~ChdReader() override;

    ChdReader(const ChdReader&) = delete;
    ChdReader& operator=(const ChdReader&) = delete;

    bool isOpen() const;
    const std::string& openError() const;
    bool read(std::uint64_t offset, std::span<std::byte> destination, std::string& error) override;

private:
    struct DataTrack {
        std::uint64_t chd_frame_offset = 0;
        std::uint64_t frames = 0;
        std::uint32_t data_offset = 0;
    };

    bool configureLayout(std::string& error);
    bool configureCdLayout(std::string& error);
    bool readHunk(std::uint32_t hunk, std::string& error);
    bool readRaw(std::uint64_t offset, std::span<std::byte> destination, std::string& error);
    bool readCd(std::uint64_t offset, std::span<std::byte> destination, std::string& error);
    bool readCdSector(std::uint64_t sector, std::span<std::byte, 2048> destination, std::string& error);

    std::FILE* source_file_ = nullptr;
    chd_file* chd_ = nullptr;
    std::uint32_t hunk_bytes_ = 0;
    std::uint64_t logical_bytes_ = 0;
    std::vector<std::byte> hunk_buffer_;
    std::uint32_t cached_hunk_ = std::numeric_limits<std::uint32_t>::max();
    bool cd_layout_ = false;
    DataTrack data_track_{};
    std::string open_error_;
};

} // namespace ps2br
