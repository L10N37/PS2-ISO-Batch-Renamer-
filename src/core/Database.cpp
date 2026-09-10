#include "core/Database.h"

#include "embedded_database.h"
#include "core/PathText.h"

#include <fstream>
#include <iterator>
#include <cctype>
#include <unordered_set>

namespace ps2br {
namespace {

void trimLineEnd(std::string& line) {
    while (!line.empty() &&
           (line.back() == '\r' || line.back() == '\n' || line.back() == '\0' ||
            line.back() == ' ' || line.back() == '\t')) {
        line.pop_back();
    }
}

void decodeLegacyBatchEscapes(std::string& title) {
    std::size_t position = 0;
    while ((position = title.find("^!", position)) != std::string::npos) {
        title.replace(position, 2, "!");
        ++position;
    }
}

bool isValidGameId(std::string_view id) {
    if (id.size() != 11 || id[4] != '_' || id[8] != '.') {
        return false;
    }
    for (std::size_t index = 0; index < id.size(); ++index) {
        if (index == 4 || index == 8) {
            continue;
        }
        const unsigned char value = static_cast<unsigned char>(id[index]);
        if (!(std::isupper(value) || std::isdigit(value))) {
            return false;
        }
    }
    return true;
}

} // namespace

bool GameDatabase::loadBuiltIn(std::string& error) {
    const bool loaded = loadText(embedded::game_database, "Built-in V4 database", error);
    if (loaded) {
        built_in_ = true;
    }
    return loaded;
}

bool GameDatabase::loadFile(const std::filesystem::path& path, std::string& error) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        error = "Could not open the selected database";
        return false;
    }
    std::string text((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const bool loaded = loadText(text, pathToUtf8(path), error);
    if (loaded) {
        built_in_ = false;
    }
    return loaded;
}

bool GameDatabase::loadText(std::string_view text, std::string source_name, std::string& error) {
    std::vector<DatabaseRecord> parsed;
    std::unordered_set<std::string> ids;
    std::size_t duplicate_count = 0;
    std::size_t cursor = 0;
    std::size_t line_number = 0;

    while (cursor <= text.size()) {
        ++line_number;
        const std::size_t end = text.find('\n', cursor);
        const std::size_t length = (end == std::string_view::npos ? text.size() : end) - cursor;
        std::string line(text.substr(cursor, length));
        trimLineEnd(line);

        if (!line.empty()) {
            if (line.size() < 13 || line[11] != ' ' ||
                !isValidGameId(std::string_view(line).substr(0, 11))) {
                error = "Database contains an invalid record on line " + std::to_string(line_number);
                return false;
            }

            DatabaseRecord record{line.substr(0, 11), line.substr(12)};
            decodeLegacyBatchEscapes(record.title);
            if (record.title.empty()) {
                error = "Database contains an empty title for " + record.game_id;
                return false;
            }
            if (!ids.insert(record.game_id).second) {
                ++duplicate_count;
            }
            parsed.push_back(std::move(record));
        }

        if (end == std::string_view::npos) {
            break;
        }
        cursor = end + 1;
    }

    if (parsed.empty()) {
        error = "Database contains no valid records";
        return false;
    }

    records_ = std::move(parsed);
    source_name_ = std::move(source_name);
    unique_id_count_ = ids.size();
    duplicate_id_count_ = duplicate_count;
    built_in_ = false;
    error.clear();
    return true;
}

const DatabaseRecord* GameDatabase::findFirst(std::string_view game_id) const {
    for (const auto& record : records_) {
        if (record.game_id == game_id) {
            return &record;
        }
    }
    return nullptr;
}

std::size_t GameDatabase::recordCount() const { return records_.size(); }
std::size_t GameDatabase::uniqueIdCount() const { return unique_id_count_; }
std::size_t GameDatabase::duplicateIdCount() const { return duplicate_id_count_; }
const std::string& GameDatabase::sourceName() const { return source_name_; }
bool GameDatabase::isBuiltIn() const { return built_in_; }

} // namespace ps2br
