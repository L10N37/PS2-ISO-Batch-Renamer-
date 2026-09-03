#include "core/Database.h"
#include "core/Identifier.h"
#include "core/IsoReader.h"
#include "core/Renamer.h"
#if PS2BR_ENABLE_CHD
#include "core/ChdReader.h"
#endif

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

int failures = 0;

void expect(bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << '\n';
        ++failures;
    }
}

void createFixture(const fs::path& path, std::string_view game_id, bool include_cnf = true) {
    constexpr std::uint32_t root_sector = 0x0105;
    constexpr std::uint32_t cnf_sector = 0x0180;
    constexpr std::size_t sector_size = 2048;
    constexpr std::size_t cnf_name_index = 0x482;
    std::vector<unsigned char> image((cnf_sector + 1) * sector_size, 0);

    image[0x8000] = 1;
    constexpr std::array<unsigned char, 5> iso_signature{'C', 'D', '0', '0', '1'};
    std::copy(iso_signature.begin(), iso_signature.end(), image.begin() + 0x8001);
    image[0x80A4] = static_cast<unsigned char>((root_sector >> 8U) & 0xFFU);
    image[0x80A5] = static_cast<unsigned char>(root_sector & 0xFFU);

    if (include_cnf) {
        const std::size_t root = root_sector * sector_size;
        const std::size_t extent = root + cnf_name_index - 31;
        image[extent] = static_cast<unsigned char>((cnf_sector >> 24U) & 0xFFU);
        image[extent + 1] = static_cast<unsigned char>((cnf_sector >> 16U) & 0xFFU);
        image[extent + 2] = static_cast<unsigned char>((cnf_sector >> 8U) & 0xFFU);
        image[extent + 3] = static_cast<unsigned char>(cnf_sector & 0xFFU);
        constexpr std::array<unsigned char, 6> name{'E', 'M', '.', 'C', 'N', 'F'};
        std::copy(name.begin(), name.end(), image.begin() + static_cast<std::ptrdiff_t>(root + cnf_name_index));

        const std::string system_cnf = "BOOT2 = cdrom0:\\" + std::string(game_id) + ";1\r\nVER = 1.00\r\n";
        std::copy(system_cnf.begin(), system_cnf.end(), image.begin() + static_cast<std::ptrdiff_t>(cnf_sector * sector_size));
    }

    std::ofstream file(path, std::ios::binary);
    file.write(reinterpret_cast<const char*>(image.data()), static_cast<std::streamsize>(image.size()));
}

void databaseTests() {
    ps2br::GameDatabase database;
    std::string error;
    expect(database.loadText(
        "SLUS_217.20 First title\r\n"
        "SLUS_217.20 Second title\r\n"
        "SLES_518.26 AFL Live 2004\r\n"
        "SLES_510.17 Scooby-Doo^! and The Night of 100 Frights  \r\n",
        "test", error), "database should load: " + error);
    expect(database.recordCount() == 4, "database should retain every record");
    expect(database.uniqueIdCount() == 3, "database should count unique IDs");
    expect(database.duplicateIdCount() == 1, "database should report duplicate IDs");
    const auto* record = database.findFirst("SLUS_217.20");
    expect(record != nullptr && record->title == "First title", "lookup must preserve first-match-wins behaviour");
    const auto* legacy_title = database.findFirst("SLES_510.17");
    expect(legacy_title != nullptr &&
               legacy_title->title == "Scooby-Doo! and The Night of 100 Frights",
           "legacy ^! escaping and trailing database padding must not appear in filenames");
}

void identifierTests(const fs::path& directory) {
    const fs::path valid = directory / "unknown.iso";
    createFixture(valid, "SLUS_217.20");
    ps2br::IsoReader reader(valid);
    const auto identification = ps2br::identifyGame(reader);
    expect(identification.success, "valid fixture should identify");
    expect(identification.game_id == "SLUS_217.20", "identifier should return the exact 11-byte ID");
    expect(identification.root_directory_offset == 0x82800, "root-directory offset should match the V2/V3 algorithm");
    expect(identification.system_cnf_offset == 0xC0000, "SYSTEM.CNF offset should match the V2/V3 algorithm");

    const fs::path invalid = directory / "invalid.iso";
    createFixture(invalid, "SLUS_217.20", false);
    ps2br::IsoReader bad_reader(invalid);
    const auto bad = ps2br::identifyGame(bad_reader);
    expect(!bad.success && bad.error.find("not found") != std::string::npos,
           "missing SYSTEM.CNF should produce a visible failure");
}

void renamerTests(const fs::path& directory) {
    ps2br::GameDatabase database;
    std::string error;
    expect(database.loadText("SLUS_217.20 Arcana Heart\r\n", "test", error), "test database should load");

    ps2br::Renamer renamer(database);
    ps2br::ScanOptions options{directory, ps2br::ImageType::Iso, ps2br::NamingMode::TitleAndGameId};
    auto results = renamer.scan(options, {}, {});
    expect(results.size() == 2, "scan should include both ISO fixtures");

    auto found = std::find_if(results.begin(), results.end(), [](const auto& item) {
        return item.game_id == "SLUS_217.20";
    });
    expect(found != results.end(), "scan should retain detected game ID");
    if (found != results.end()) {
        expect(found->destination_path.filename() == "Arcana Heart (SLUS_217.20).iso",
               "GameName naming mode should append the ID in parentheses");
    }

    const auto summary = renamer.apply(results, {}, {});
    expect(summary.renamed == 1, "one valid fixture should be renamed");
    expect(summary.failed == 1, "one invalid fixture should remain visibly failed");
    expect(fs::exists(directory / "Arcana Heart (SLUS_217.20).iso"), "renamed file should exist");
    expect(!fs::exists(directory / "failed.txt"), "results must stay in memory unless the user saves a report");
    expect(!fs::exists(directory / "PS2_iso_files.txt"), "scanning must not leave a generated file list behind");
}

void safetyTests(const fs::path& root) {
    std::string error;

    const fs::path invalid_title_directory = root / "invalid-title";
    fs::create_directories(invalid_title_directory);
    createFixture(invalid_title_directory / "game.iso", "SLUS_217.20");
    ps2br::GameDatabase invalid_title_database;
    expect(invalid_title_database.loadText("SLUS_217.20 Broken * Title\r\n", "test", error),
           "database with a legacy-invalid title should still load");
    ps2br::Renamer invalid_title_renamer(invalid_title_database);
    auto invalid_title_results = invalid_title_renamer.scan(
        {invalid_title_directory, ps2br::ImageType::Iso, ps2br::NamingMode::TitleOnly}, {}, {});
    expect(invalid_title_results.size() == 1 && invalid_title_results[0].status == ps2br::ResultStatus::Failed,
           "a title invalid on Windows should be shown as failed during preview on every platform");
    expect(fs::exists(invalid_title_directory / "game.iso"), "an invalid database title must not rename the source");

    const fs::path collision_directory = root / "collision";
    fs::create_directories(collision_directory);
    createFixture(collision_directory / "game.iso", "SLUS_217.20");
    createFixture(collision_directory / "Arcana Heart.iso", "SLUS_217.20");
    ps2br::GameDatabase collision_database;
    expect(collision_database.loadText("SLUS_217.20 Arcana Heart\r\n", "test", error),
           "collision test database should load");
    ps2br::Renamer collision_renamer(collision_database);
    auto collision_results = collision_renamer.scan(
        {collision_directory, ps2br::ImageType::Iso, ps2br::NamingMode::TitleOnly}, {}, {});
    const auto collision_summary = collision_renamer.apply(collision_results, {}, {});
    expect(collision_summary.failed == 1, "an existing destination should produce a visible failure");
    expect(fs::exists(collision_directory / "game.iso"), "the source must remain when a destination exists");
}

#if PS2BR_ENABLE_CHD && defined(PS2BR_CHDMAN_EXECUTABLE)
void chdRoundTripTest(const fs::path& root) {
    const fs::path directory = root / "chd-round-trip";
    fs::create_directories(directory);
    const fs::path iso = directory / "fixture.iso";
    const fs::path cue = directory / "fixture.cue";
    const fs::path chd = directory / "fixture.chd";
    createFixture(iso, "SLUS_217.20");

    std::ofstream cue_file(cue);
    cue_file << "FILE \"fixture.iso\" BINARY\n"
             << "  TRACK 01 MODE1/2048\n"
             << "    INDEX 01 00:00:00\n";
    cue_file.close();

    const std::string command = std::string("\"") + PS2BR_CHDMAN_EXECUTABLE +
        "\" createcd -i \"" + cue.string() + "\" -o \"" + chd.string() + "\" >/dev/null 2>&1";
    expect(std::system(command.c_str()) == 0, "chdman should create the CHD round-trip fixture");
    if (!fs::exists(chd)) return;

    ps2br::ChdReader reader(chd);
    expect(reader.isOpen(), "native CHD reader should open a chdman image: " + reader.openError());
    if (!reader.isOpen()) return;
    const auto identification = ps2br::identifyGame(reader);
    expect(identification.success, "native CHD reader should expose the original disc bytes");
    expect(identification.game_id == "SLUS_217.20", "CHD and ISO identification should return the same game ID");
}
#endif

} // namespace

int main() {
    databaseTests();
    const fs::path directory = fs::temp_directory_path() / "ps2br-v4-core-tests";
    std::error_code ec;
    fs::remove_all(directory, ec);
    fs::create_directories(directory, ec);
    expect(!ec, "temporary test directory should be created");
    identifierTests(directory);
    renamerTests(directory);
    safetyTests(directory);
#if PS2BR_ENABLE_CHD && defined(PS2BR_CHDMAN_EXECUTABLE)
    chdRoundTripTest(directory);
#endif
    fs::remove_all(directory, ec);

    if (failures == 0) {
        std::cout << "All PS2 Batch Renamer V4 core tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
