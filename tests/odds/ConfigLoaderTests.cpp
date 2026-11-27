// Tests for parsePackConfig(): JSON text -> PackConfig (design doc Section 7)

#include "odds/ConfigError.h"
#include "odds/ConfigLoader.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace odds = ygo::odds;

namespace {

constexpr double kTolerance = 1e-12;

// LOB's config exactly as in the design doc
// R"( ... )" is a raw string literal: quotes and newlines are kept as typed, no \" escaping needed
constexpr std::string_view kLobJson = R"(
{
  "set_code": "LOB",
  "total_cards_per_pack": 9,
  "filler_slots": 8,
  "filler_pool": "Common",
  "guarantee_slots": [
    {
      "slot_index": 9,
      "table": [
        { "rarity": "Secret Rare", "odds_denominator": 36 },
        { "rarity": "Ultra Rare",  "odds_denominator": 12 },
        { "rarity": "Super Rare",  "odds_denominator": 6 },
        { "rarity": "Rare",        "remainder": true }
      ]
    }
  ]
}
)";

// A valid pack split around its table entries: packWithTable() puts a test's entries between the two halves
constexpr std::string_view kPackBeforeEntries = R"(
{
  "set_code": "TST",
  "total_cards_per_pack": 9,
  "filler_slots": 8,
  "filler_pool": "Common",
  "guarantee_slots": [
    {
      "slot_index": 9,
      "table": [
)";

constexpr std::string_view kPackAfterEntries = R"(
      ]
    }
  ]
}
)";

// A valid pack around the given table entries, so a test only spells out the part it's about
std::string packWithTable(std::string_view tableEntries)
{
    return std::string(kPackBeforeEntries) + std::string(tableEntries) + std::string(kPackAfterEntries);
}

// Parses json, expects a ConfigError, and checks its message contains `expected`
void expectErrorContaining(const std::string& json, std::string_view expected)
{
    try {
        odds::parsePackConfig(json);
        FAIL() << "expected ConfigError for: " << json;
    } catch (const odds::ConfigError& error) {
        const std::string errorMessage = error.what();
        EXPECT_NE(errorMessage.find(expected), std::string::npos) << errorMessage;
    }
}

// The repo's data/ folder; YGO_DATA_DIR comes from tests/CMakeLists.txt
// The / operator joins paths with the right separator for the platform
std::filesystem::path dataFile(std::string_view fileName)
{
    return std::filesystem::path(YGO_DATA_DIR) / fileName;
}

// Loads a file and returns the ConfigError message, or "" if it loaded fine
// Returning the message (instead of checking inside) lets a test clean up before its EXPECTs
std::string loadPackConfigError(const std::filesystem::path& path)
{
    try {
        odds::loadPackConfig(path);
    } catch (const odds::ConfigError& error) {
        return error.what();
    }
    return {};
}

} // namespace

TEST(ConfigLoader, ParsesLobConfig)
{
    const odds::PackConfig config = odds::parsePackConfig(kLobJson);

    EXPECT_EQ(config.setCode, "LOB");
    EXPECT_EQ(config.totalCardsPerPack, 9);
    EXPECT_EQ(config.fillerSlots, 8);
    EXPECT_EQ(config.fillerPool, "Common");
    ASSERT_EQ(config.guaranteeSlots.size(), 1u);

    const odds::SlotTable& slot = config.guaranteeSlots[0];
    EXPECT_EQ(slot.slotIndex, 9);
    ASSERT_EQ(slot.entries.size(), 4u);
    EXPECT_EQ(slot.entries[0].rarity, "Secret Rare");
    EXPECT_NEAR(slot.entries[0].probability, 1.0 / 36, kTolerance);
    EXPECT_EQ(slot.entries[2].rarity, "Super Rare");
    EXPECT_NEAR(slot.entries[2].probability, 1.0 / 6, kTolerance);
    EXPECT_EQ(slot.entries[3].rarity, "Rare");
    EXPECT_NEAR(slot.entries[3].probability, 26.0 / 36, kTolerance);
}

TEST(ConfigLoader, RejectsInvalidJson)
{
    expectErrorContaining(R"({ "set_code": "LOB", )", "not valid JSON");
}

TEST(ConfigLoader, RejectsNonObjectRoot)
{
    expectErrorContaining("[1, 2, 3]", "must be an object");
}

TEST(ConfigLoader, RejectsMissingField)
{
    expectErrorContaining(R"({ "set_code": "LOB" })", "missing field \"total_cards_per_pack\"");
}

TEST(ConfigLoader, RejectsWrongFieldTypes)
{
    // A quoted number is a string in JSON, not a number
    expectErrorContaining(R"({ "set_code": "LOB", "total_cards_per_pack": "9" })",
                          "\"total_cards_per_pack\" must be a whole number");
    expectErrorContaining(R"({ "set_code": "LOB", "total_cards_per_pack": 9.5 })",
                          "\"total_cards_per_pack\" must be a whole number");
    expectErrorContaining(R"({ "set_code": 1 })", "\"set_code\" must be a string");
    expectErrorContaining(R"({ "set_code": "LOB", "total_cards_per_pack": 9, "filler_slots": 8,
                               "filler_pool": "Common", "guarantee_slots": {} })",
                          "\"guarantee_slots\" must be an array");
    expectErrorContaining(packWithTable(R"({ "rarity": "Super Rare", "odds_denominator": "6" })"),
                          "\"odds_denominator\" must be a number");
}

TEST(ConfigLoader, RejectsEmptyString)
{
    expectErrorContaining(R"({ "set_code": "" })", "\"set_code\" must not be empty");
}

TEST(ConfigLoader, RejectsEntryWithoutOddsKey)
{
    expectErrorContaining(packWithTable(R"({ "rarity": "Rare" })"), "needs exactly one of");
}

TEST(ConfigLoader, RejectsWeightKey)
{
    // Weights aren't supported: "weight" is just an unknown key, so the entry has no odds at all
    expectErrorContaining(packWithTable(R"({ "rarity": "Rare", "weight": 70 })"), "(got 0)");
}

TEST(ConfigLoader, RejectsEntryWithSeveralOddsKeys)
{
    expectErrorContaining(packWithTable(R"({ "rarity": "Rare", "odds_denominator": 6, "remainder": true })"),
                          "(got 2)");
}

TEST(ConfigLoader, RejectsRemainderFalse)
{
    expectErrorContaining(packWithTable(R"({ "rarity": "Super Rare", "odds_denominator": 6 },
                                           { "rarity": "Rare", "remainder": false })"),
                          "\"remainder\" must be true");
}

TEST(ConfigLoader, ErrorNamesTheTableEntry)
{
    // Second entry (index 1) of the first slot is the bad one
    expectErrorContaining(packWithTable(R"({ "rarity": "Rare", "remainder": true }, { "odds_denominator": 6 })"),
                          "guarantee_slots[0].table[1]: missing field \"rarity\"");
}

TEST(ConfigLoader, NormalizeRulesStillApply)
{
    // Well-formed JSON, but 1/2 + 1/2 + 1/2 > 1, which normalize() rejects
    expectErrorContaining(packWithTable(R"({ "rarity": "A", "odds_denominator": 2 },
                                           { "rarity": "B", "odds_denominator": 2 },
                                           { "rarity": "C", "odds_denominator": 2 },
                                           { "rarity": "Rare", "remainder": true })"),
                          "more than 1");
}

// --- Loading from a file ---

TEST(ConfigLoader, LoadsLobFile)
{
    const odds::PackConfig config = odds::loadPackConfig(dataFile("LOB.json"));

    EXPECT_EQ(config.setCode, "LOB");
    EXPECT_EQ(config.totalCardsPerPack, 9);
    EXPECT_EQ(config.fillerSlots, 8);
    EXPECT_EQ(config.fillerPool, "Common");
    ASSERT_EQ(config.guaranteeSlots.size(), 1u);

    const odds::SlotTable& slot = config.guaranteeSlots[0];
    EXPECT_EQ(slot.slotIndex, 9);
    ASSERT_EQ(slot.entries.size(), 4u);
    EXPECT_NEAR(slot.entries[0].probability, 1.0 / 36, kTolerance);
    EXPECT_NEAR(slot.entries[1].probability, 1.0 / 12, kTolerance);
    EXPECT_NEAR(slot.entries[2].probability, 1.0 / 6, kTolerance);
    EXPECT_NEAR(slot.entries[3].probability, 26.0 / 36, kTolerance);
}

TEST(ConfigLoader, RejectsMissingFile)
{
    const std::filesystem::path path = dataFile("NO_SUCH_SET.json");

    const std::string errorMessage = loadPackConfigError(path);

    EXPECT_NE(errorMessage.find(path.string()), std::string::npos) << errorMessage;
    EXPECT_NE(errorMessage.find("can't open file"), std::string::npos) << errorMessage;
}

TEST(ConfigLoader, FileErrorNamesTheFile)
{
    // A bad config written to the system's temp folder, so the repo's data/ stays untouched
    const std::filesystem::path path = std::filesystem::temp_directory_path() / "ygo_config_loader_test.json";
    {
        // Braces end `out`'s scope here, so its destructor flushes and closes the file before it's loaded
        std::ofstream out(path);
        out << R"({ "set_code": "BAD" })";
    }

    const std::string errorMessage = loadPackConfigError(path);
    std::filesystem::remove(path); // before the EXPECTs, so the file is cleaned up even if they fail

    EXPECT_NE(errorMessage.find(path.string()), std::string::npos) << errorMessage;
    EXPECT_NE(errorMessage.find("missing field \"total_cards_per_pack\""), std::string::npos) << errorMessage;
}
