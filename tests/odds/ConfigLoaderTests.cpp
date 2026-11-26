// Tests for parsePackConfig(): JSON text -> PackConfig (design doc Section 7)

#include "odds/ConfigError.h"
#include "odds/ConfigLoader.h"

#include <gtest/gtest.h>

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

// A valid pack around the given table entries, so a test only spells out the part it's about
std::string packWithTable(std::string_view tableEntries)
{
    return std::string(R"({ "set_code": "TST", "total_cards_per_pack": 9, "filler_slots": 8,
                            "filler_pool": "Common",
                            "guarantee_slots": [ { "slot_index": 9, "table": [ )")
           + std::string(tableEntries) + "] } ] }";
}

// Parses json, expects a ConfigError, and checks its message contains `expected`
void expectErrorContaining(const std::string& json, std::string_view expected)
{
    try {
        odds::parsePackConfig(json);
        FAIL() << "expected ConfigError for: " << json;
    } catch (const odds::ConfigError& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find(expected), std::string::npos) << message;
    }
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
