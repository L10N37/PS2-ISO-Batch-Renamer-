#include "core/Database.h"
#include "core/Identifier.h"
#include "core/IsoReader.h"
#include "core/OnlineDatabase.h"
#include "core/Renamer.h"
#if PS2BR_ENABLE_CHD
#include "core/ChdReader.h"
#endif

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
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

    ps2br::GameDatabase alphanumeric;
    expect(alphanumeric.loadText("KOEI_SP0.02 Koei Spring Pack\r\n", "test", error),
           "valid alphanumeric product IDs should load: " + error);
    ps2br::GameDatabase malformed;
    expect(!malformed.loadText("SLES-501.78 Oni\r\n", "test", error),
           "a database record with a hyphen in place of the canonical underscore must fail");
    expect(error.find("line 1") != std::string::npos,
           "a malformed database record should report its physical line number");

    ps2br::GameDatabase built_in;
    expect(built_in.loadBuiltIn(error), "built-in database should load: " + error);
    expect(built_in.recordCount() == 13442, "built-in database should contain the audited record count");
    expect(built_in.uniqueIdCount() == 13442, "every built-in database ID should be unique");
    expect(built_in.duplicateIdCount() == 0, "built-in database should contain no duplicate IDs");
    constexpr std::array<std::pair<std::string_view, std::string_view>, 62> audited_titles{{
        {"PCPX_966.49", "Gran Turismo 4 - First Preview"},
        {"SCAJ_200.12", "Venus & Braves"},
        {"SCKA_200.58", "Action Romance Bumpy Trot"},
        {"SCKA_200.62", "Saru! Get You! 3"},
        {"SLAJ_250.23", "Shin Sangoku Musou 3 - Moushouden"},
        {"SLES_503.82", "Silent Hill 2"},
        {"SLES_512.27", "Lara Croft Tomb Raider - The Angel of Darkness"},
        {"SLES_512.52", "The Lord of the Rings - The Two Towers"},
        {"SLES_512.58", "007 - Nightfire [Platinum]"},
        {"SLES_518.97", "The Simpsons - Hit & Run"},
        {"SLES_520.05", "007 - Everything or Nothing"},
        {"SLES_525.88", "Mercenaries - Playground of Destruction"},
        {"SLES_525.90", "Mercenaries - Playground of Destruction"},
        {"SLES_528.01", "The Lord of the Rings - The Third Age"},
        {"SLES_529.74", "GoldenEye - Rogue Agent"},
        {"SLES_533.42", "Cricket 2005"},
        {"SLES_533.57", "21 Card Games"},
        {"SLES_533.66", "Killer7"},
        {"SLES_540.16", "AND 1 Streetball"},
        {"SLES_541.47", "Ice Age 2 - The Meltdown"},
        {"SLES_542.09", "The Sopranos - Road to Respect"},
        {"SLES_543.47", "The Sims 2 - Pets"},
        {"SLES_548.15", "The Legend of Spyro - The Eternal Night"},
        {"SLES_549.98", "Mercenaries 2 - L'enfer des Favelas"},
        {"SLES_820.18", "Cy Girls (Ice Disc)"},
        {"SLES_820.20", "Cy Girls (Ice Disc)"},
        {"SLES_820.21", "Cy Girls (Aska Disc)"},
        {"SLKA_251.03", "Shinseiki Evangelion 2 - Evangelions"},
        {"SLKA_251.31", "Jyuouki - Project Altered Beast"},
        {"SLKA_253.17", "Jin Samguk Mussang 3"},
        {"SLPM_650.40", "The Fear [Disc 1]"},
        {"SLPS_251.95", "Venus & Braves - Majo to Megami to Horobi no Yogen [Premium Box]"},
        {"SLPS_251.96", "Venus & Braves - Majo to Megami to Horobi no Yogen"},
        {"SLPS_254.85", "Kidou Senshi Gundam - Ver. 1.5 [Gundam the Best]"},
        {"SLPS_732.36", "Venus & Braves - Majo to Megami to Horobi no Yogen [PlayStation 2 The Best]"},
        {"SLES_546.04", "\xC2\xA1Qu\xC3\xA9 Pasa Neng! El Videojuego"},
        {"SLPM_550.11", "Poi Hito Natsu no Keiken! [Limited Edition]"},
        {"SLPM_550.12", "Ppoi! Hito Natsu no Keiken!"},
        {"SLPM_551.32", "Kira Kira - Rock 'N' Roll Show [First Print Limited Edition]"},
        {"SLPM_551.33", "Kira Kira - Rock 'n' Roll Show"},
        {"SLPM_551.49", "Loveroot Zero - Kiss Kiss Labyrinth"},
        {"SLPM_552.79", "Otometeki Koi Kakumei - Love Revo!! [Best Edition]"},
        {"SLPM_552.88", "Shin Koihime Musou - Otome Ryouran Sangokushi Engi"},
        {"SLPM_620.93", "Simple 2000 Series Ultimate Vol. 1 - Love Smash!"},
        {"SLPS_200.61", "Mamimume Mogacho no Print Hour"},
        {"SLPS_258.83", "Katekyoo Hitman Reborn! Nerae! Ring x Vongola Trainers"},
        {"SLPS_258.99", "Katekyoo Hitman Reborn! Let's Ansatsu! Nerawareta 10-daime! [Best Collection]"},
        {"SLPS_259.09", "Hisshou Pachinko Pachi-Slot Kouryaku Series Vol. 13 - Shin Seiki Evangelion - Yakusoku no Toki"},
        {"SLPS_259.11", "Hisshou Pachinko Pachi-Slot Kouryaku Series Vol. 11 - Shin Seiki Evangelion - Magokoro o, Kimi ni"},
        {"SLPS_259.31", "Katekyoo Hitman Reborn! Nerae! Ring x Vongola Trainers [Best Collection]"},
        {"SLPS_259.42", "Hisshou Pachinko Pachi-Slot Kouryaku Series Vol. 14 - CR Shin Seiki Evangelion - Saigo no Shisha"},
        {"SLPS_259.43", "Hisshou Pachinko Pachi-Slot Kouryaku Series Vol. 14 - CR Shin Seiki Evangelion - Saigo no Shisha [Limited Special Box]"},
        {"SLES_511.68", "AFL Live 2003"},
        {"SLES_518.26", "AFL Live 2004"},
        {"SLES_519.03", "AFL Live 2004 - Aussie Rules Football"},
        {"SLES_504.87", "Looney Tunes - Space Race"},
        {"SLPM_620.89", "Wacky Races Starring Dastardly & Muttley [Cancelled Japanese Release]"},
        {"SLPM_650.49", "Beatmania II DX 5th Style - New Songs Collection"},
        {"SLPM_650.52", "Gitadora! GuitarFreaks 4th Mix & DrumMania 3rd Mix"},
        {"SLKA_253.23", "FIFA 06"},
        {"SLPM_602.65", "Steamboy [Trial]"},
        {"SCES_609.71", "Kingdom Hearts II - Final Mix + [English Translation]"},
    }};
    for (const auto& [game_id, expected_title] : audited_titles) {
        const auto* audited = built_in.findFirst(game_id);
        expect(audited != nullptr && audited->title == expected_title,
               "built-in database should contain the audited title for " + std::string(game_id));
    }
    constexpr std::array<std::string_view, 36> unofficial_ids{
        "USGR_000.00", "USGR_000.01", "USGR_000.02", "USGR_000.03", "USGR_000.04", "USGR_000.05",
        "USGR_000.06", "USGR_000.07", "USGR_000.08", "USGR_000.09", "USGR_000.10", "USGR_000.11",
        "USGR_000.12", "USGR_000.13", "USGR_000.15", "USGR_000.16", "USGR_000.18", "USGR_000.19",
        "USGR_000.20", "USGR_000.21", "USGR_000.22", "USGR_000.23", "USGR_000.24", "USGR_000.25",
        "USGR_000.26", "USGR_000.34", "USGR_000.35", "USGR_000.36", "USGR_000.37", "USGR_000.38",
        "USGR_000.39", "USGR_000.40", "USGR_000.41", "USGR_999.97", "USGR_999.98", "USGR_999.99",
    };
    for (const auto id : unofficial_ids) {
        expect(built_in.findFirst(id) != nullptr, "built-in database should include documented unofficial ID " +
                                                     std::string(id));
    }
    const auto* taito = built_in.findFirst("USGR_000.25");
    expect(taito != nullptr && taito->title == "Taito Legends 1 & 2",
           "USGR_000.25 should resolve to the unofficial Taito Legends 1 & 2 compilation");
    const auto* retail_kingdom_hearts = built_in.findFirst("SLPM_666.75");
    expect(retail_kingdom_hearts != nullptr &&
               retail_kingdom_hearts->title == "Kingdom Hearts II - Final Mix +",
           "the retail ID must not inherit the unofficial English-translation suffix");
}

std::string onlineFixture(std::size_t records, bool duplicate_last = false) {
    std::string text;
    text.reserve(records * 32);
    for (std::size_t index = 0; index < records; ++index) {
        const std::size_t serial = duplicate_last && index + 1 == records ? 0 : index;
        char game_id[32]{};
        std::snprintf(game_id, sizeof(game_id), "TST0_%03zu.%02zu", serial / 100, serial % 100);
        text += game_id;
        text += " Validation Fixture ";
        text += std::to_string(index);
        text += '\n';
    }
    return text;
}

void onlineDatabaseTests() {
    std::string error;
    const std::string complete = onlineFixture(ps2br::kOnlineDatabaseMinimumRecords);
    expect(ps2br::validateOnlineDatabase(complete, error),
           "a complete unique online database should validate: " + error);

    const std::string incomplete = onlineFixture(ps2br::kOnlineDatabaseMinimumRecords - 1);
    expect(!ps2br::validateOnlineDatabase(incomplete, error) &&
               error.find("incomplete") != std::string::npos,
           "an incomplete download must be rejected");

    const std::string duplicate = onlineFixture(ps2br::kOnlineDatabaseMinimumRecords, true);
    expect(!ps2br::validateOnlineDatabase(duplicate, error) &&
               error.find("duplicate") != std::string::npos,
           "a downloaded database with duplicate IDs must be rejected");

    std::string malformed = complete;
    malformed.replace(0, 11, "TST0-000.00");
    expect(!ps2br::validateOnlineDatabase(malformed, error) &&
               error.find("line 1") != std::string::npos,
           "a malformed downloaded ID must be rejected with a line number");

    const std::string oversized(ps2br::kOnlineDatabaseMaximumBytes + 1, 'x');
    expect(!ps2br::validateOnlineDatabase(oversized, error) &&
               error.find("4 MiB") != std::string::npos,
           "an oversized download must be rejected before parsing");

    ps2br::GameDatabase active;
    expect(active.loadText("KEEP_000.01 Offline Fallback\n", "fallback", error),
           "fallback fixture should load");
    if (ps2br::validateOnlineDatabase(incomplete, error)) {
        active.loadText(incomplete, "unexpected", error);
    }
    const auto* fallback = active.findFirst("KEEP_000.01");
    expect(fallback != nullptr && fallback->title == "Offline Fallback",
           "a rejected online database must leave the active fallback unchanged");

#if !PS2BR_ENABLE_ONLINE_DATABASE
    const auto disabled = ps2br::downloadOnlineDatabase();
    expect(!disabled.success && disabled.error.find("disabled") != std::string::npos,
           "offline builds should return the explicit disabled result without networking");
#endif
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
    onlineDatabaseTests();
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
        std::cout << "All PS2 Batch Renamer V4.1 core tests passed.\n";
    }
    return failures == 0 ? 0 : 1;
}
