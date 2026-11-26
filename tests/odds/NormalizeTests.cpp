// Tests for normalize(): raw config odds -> probabilities (design doc Section 7)

#include "odds/ConfigError.h"
#include "odds/Normalize.h"

#include <gtest/gtest.h>

#include <limits>
#include <string>
#include <utility>
#include <vector>

namespace odds = ygo::odds;

namespace {

// Short builders, so each test shows only the entries it's about
odds::RawRarityEntry oneIn(const std::string& rarity, double n)
{
    return {.rarity = rarity, .kind = odds::OddsKind::OneInN, .value = n};
}

odds::RawRarityEntry remainder(const std::string& rarity)
{
    return {.rarity = rarity, .kind = odds::OddsKind::Remainder};
}

// A slot 9 table holding the given entries
// entries is taken by value and moved in: the caller's temporary list is handed over, not copied
odds::RawSlotTable tableOf(std::vector<odds::RawRarityEntry> entries)
{
    return {.slotIndex = 9, .entries = std::move(entries)};
}

constexpr double kNaN = std::numeric_limits<double>::quiet_NaN();
constexpr double kInfinity = std::numeric_limits<double>::infinity();

// Computed doubles are compared with a tolerance, never with ==
constexpr double kTolerance = 1e-12;

// LOB's guarantee slot as the config file writes it
// A function, not a shared global, so every test gets its own fresh copy
odds::RawSlotTable lobRawSlot()
{
    return {
        .slotIndex = 9,
        .entries = {
            {.rarity = "Secret Rare", .kind = odds::OddsKind::OneInN, .value = 36},
            {.rarity = "Ultra Rare", .kind = odds::OddsKind::OneInN, .value = 12},
            {.rarity = "Super Rare", .kind = odds::OddsKind::OneInN, .value = 6},
            {.rarity = "Rare", .kind = odds::OddsKind::Remainder},
        },
    };
}

} // namespace

TEST(Normalize, LobOneInNOddsBecomeProbabilities)
{
    const odds::SlotTable slot = odds::normalize(lobRawSlot());

    EXPECT_EQ(slot.slotIndex, 9);
    // ASSERT stops this test if it fails: indexing entries[3] below would be out of bounds
    ASSERT_EQ(slot.entries.size(), 4u); // u suffix for unsigned int, .size() returns std::vector<...>::size_type

    EXPECT_EQ(slot.entries[0].rarity, "Secret Rare");
    EXPECT_NEAR(slot.entries[0].probability, 1.0 / 36, kTolerance);
    EXPECT_EQ(slot.entries[1].rarity, "Ultra Rare");
    EXPECT_NEAR(slot.entries[1].probability, 1.0 / 12, kTolerance);
    EXPECT_EQ(slot.entries[2].rarity, "Super Rare");
    EXPECT_NEAR(slot.entries[2].probability, 1.0 / 6, kTolerance);
    EXPECT_EQ(slot.entries[3].rarity, "Rare");
    EXPECT_NEAR(slot.entries[3].probability, 26.0 / 36, kTolerance);
}

TEST(Normalize, ProbabilitiesAddUpToOne)
{
    const odds::SlotTable slot = odds::normalize(lobRawSlot());

    double total = 0.0;
    for (const auto& entry : slot.entries) {
        total += entry.probability;
    }

    EXPECT_NEAR(total, 1.0, kTolerance);
}

// --- Valid edge cases ---

TEST(Normalize, RemainderAloneIsCertain)
{
    const odds::SlotTable slot = odds::normalize(tableOf({remainder("Rare")}));

    ASSERT_EQ(slot.entries.size(), 1u);
    EXPECT_NEAR(slot.entries[0].probability, 1.0, kTolerance);
}

TEST(Normalize, OneInNSummingToOneWithRoundingIsAccepted)
{
    // Exactly 1 on paper, but 1.0000000000000002 in doubles: the tolerance must let it through,
    // and the remainder must be clamped to 0, not a tiny negative
    const odds::RawSlotTable raw = tableOf({oneIn("A", 2), oneIn("B", 12), oneIn("C", 3), oneIn("D", 20),
                                            oneIn("E", 30), remainder("Rare")});

    const odds::SlotTable slot = odds::normalize(raw);

    ASSERT_EQ(slot.entries.size(), 6u);
    EXPECT_GE(slot.entries[5].probability, 0.0);
    EXPECT_NEAR(slot.entries[5].probability, 0.0, kTolerance);
}

// --- Rejected tables (each one is a rule listed in Normalize.h) ---

TEST(Normalize, RejectsEmptyTable)
{
    EXPECT_THROW(odds::normalize(tableOf({})), odds::ConfigError);
}

TEST(Normalize, RejectsEntryWithoutRarity)
{
    EXPECT_THROW(odds::normalize(tableOf({remainder("")})), odds::ConfigError);
}

TEST(Normalize, RejectsOneInNBelowOne)
{
    for (const double bad : {0.0, 0.5, -6.0, kNaN, kInfinity}) {
        // SCOPED_TRACE adds the value to any failure message, so you can tell which one failed
        SCOPED_TRACE(bad);
        EXPECT_THROW(odds::normalize(tableOf({oneIn("Super Rare", bad), remainder("Rare")})),
                     odds::ConfigError);
    }
}

TEST(Normalize, RejectsOneInNWithoutRemainder)
{
    EXPECT_THROW(odds::normalize(tableOf({oneIn("Super Rare", 6)})), odds::ConfigError);
}

TEST(Normalize, RejectsTwoRemainders)
{
    EXPECT_THROW(odds::normalize(tableOf({oneIn("Super Rare", 6), remainder("Rare"), remainder("Common")})),
                 odds::ConfigError);
}

TEST(Normalize, RejectsOneInNAddingUpToMoreThanOne)
{
    EXPECT_THROW(odds::normalize(tableOf({oneIn("Super Rare", 2), oneIn("Ultra Rare", 1), remainder("Rare")})),
                 odds::ConfigError);
}

TEST(Normalize, ErrorMessageNamesTheSlotAndRarity)
{
    try {
        odds::normalize(tableOf({oneIn("Super Rare", 0), remainder("Rare")}));
        FAIL() << "expected ConfigError";
    } catch (const odds::ConfigError& error) {
        const std::string message = error.what();
        EXPECT_NE(message.find("slot 9"), std::string::npos) << message;
        EXPECT_NE(message.find("Super Rare"), std::string::npos) << message;
    }
}
