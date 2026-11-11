// Tests for normalize(): raw config odds -> probabilities (design doc Section 7)

#include "odds/Normalize.h"

#include <gtest/gtest.h>

namespace odds = ygo::odds;

namespace {

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
    ASSERT_EQ(slot.entries.size(), 4u);

    EXPECT_EQ(slot.entries[0].rarity, "Secret Rare");
    EXPECT_NEAR(slot.entries[0].probability, 1.0 / 36, kTolerance);
    EXPECT_EQ(slot.entries[1].rarity, "Ultra Rare");
    EXPECT_NEAR(slot.entries[1].probability, 1.0 / 12, kTolerance);
    EXPECT_EQ(slot.entries[2].rarity, "Super Rare");
    EXPECT_NEAR(slot.entries[2].probability, 1.0 / 6, kTolerance);
    EXPECT_EQ(slot.entries[3].rarity, "Rare");
    EXPECT_NEAR(slot.entries[3].probability, 26.0 / 36, kTolerance);
}

TEST(Normalize, WeightsAreDividedByTheirSum)
{
    const odds::RawSlotTable raw{
        .slotIndex = 9,
        .entries = {
            {.rarity = "Rare", .kind = odds::OddsKind::Weight, .value = 70},
            {.rarity = "Super", .kind = odds::OddsKind::Weight, .value = 20},
            {.rarity = "Ultra", .kind = odds::OddsKind::Weight, .value = 10},
        },
    };

    const odds::SlotTable slot = odds::normalize(raw);

    ASSERT_EQ(slot.entries.size(), 3u);
    EXPECT_NEAR(slot.entries[0].probability, 0.7, kTolerance);
    EXPECT_NEAR(slot.entries[1].probability, 0.2, kTolerance);
    EXPECT_NEAR(slot.entries[2].probability, 0.1, kTolerance);
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
