#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace ps2br {

struct DatabaseRecord {
    std::string game_id;
    std::string title;
};

class GameDatabase {
public:
    bool loadBuiltIn(std::string& error);
    bool loadFile(const std::filesystem::path& path, std::string& error);
    bool loadText(std::string_view text, std::string source_name, std::string& error);

    const DatabaseRecord* findFirst(std::string_view game_id) const;
    std::size_t recordCount() const;
    std::size_t uniqueIdCount() const;
    std::size_t duplicateIdCount() const;
    const std::string& sourceName() const;
    bool isBuiltIn() const;

private:
    std::vector<DatabaseRecord> records_;
    std::string source_name_;
    std::size_t unique_id_count_ = 0;
    std::size_t duplicate_id_count_ = 0;
    bool built_in_ = false;
};

} // namespace ps2br
