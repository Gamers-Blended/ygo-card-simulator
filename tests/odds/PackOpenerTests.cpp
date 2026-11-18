// Tests for PackOpener: rolling one slot and laying out a whole pack
// Long-run frequency tests (fixed seed, tolerance) come in a later step

#include "odds/ConfigError.h"
#include "odds/PackOpener.h"

#include <gtest/gtest.h>

#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace odds = ygo::odds;

namespace {

// Any fixed seed works; these tests don't depend on which numbers come out
constexpr std::uint32_t kSeed = 42;

// A slot that always rolls the given rarity (probability 1), so results don't depend on the RNG
odds::SlotTable certainSlot(int slotIndex, const std::string& rarity)
{
    return {.slotIndex = slotIndex, .entries = {{.rarity = rarity, .probability = 1.0}}};
}

// A 9-card pack shaped like LOB, with its guarantee slot replaced by the given slots
odds::PackConfig packWith(int fillerSlots, std::vector<odds::SlotTable> guaranteeSlots)
{
    return {
        .setCode = "TEST",
        .totalCardsPerPack = 9,
        .fillerSlots = fillerSlots,
        .fillerPool = "Common",
        .guaranteeSlots = std::move(guaranteeSlots),
    };
}

} // namespace

// --- rollSlot ---

TEST(PackOpener, RollSlotReturnsACertainRarity)
{
    odds::PackOpener opener{kSeed};

    EXPECT_EQ(opener.rollSlot(certainSlot(9, "Rare")), "Rare");
}

TEST(PackOpener, RollSlotRejectsEmptyTable)
{
    odds::PackOpener opener{kSeed};

    EXPECT_THROW(opener.rollSlot(odds::SlotTable{.slotIndex = 9}), std::invalid_argument);
}

TEST(PackOpener, SameSeedGivesSameRolls)
{
    const odds::SlotTable slot{
        .slotIndex = 9,
        .entries = {{.rarity = "Rare", .probability = 0.5}, {.rarity = "Super Rare", .probability = 0.5}},
    };
    odds::PackOpener first{kSeed};
    odds::PackOpener second{kSeed};

    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(first.rollSlot(slot), second.rollSlot(slot)) << "roll " << i;
    }
}

// --- openPack layout ---

TEST(PackOpener, LobShapedPackIsEightFillerThenTheGuaranteeSlot)
{
    odds::PackOpener opener{kSeed};

    const std::vector<std::string> pack = opener.openPack(packWith(8, {certainSlot(9, "Rare")}));

    ASSERT_EQ(pack.size(), 9u);
    for (std::size_t i = 0; i < 8; ++i) {
        EXPECT_EQ(pack[i], "Common") << "position " << i;
    }
    EXPECT_EQ(pack[8], "Rare");
}

TEST(PackOpener, GuaranteeSlotsGoToTheirSlotIndex)
{
    odds::PackOpener opener{kSeed};

    // slotIndex is 1-based, so slot 1 is pack[0] and slot 5 is pack[4]
    const std::vector<std::string> pack =
        opener.openPack(packWith(7, {certainSlot(1, "Super Rare"), certainSlot(5, "Rare")}));

    ASSERT_EQ(pack.size(), 9u);
    EXPECT_EQ(pack[0], "Super Rare");
    EXPECT_EQ(pack[4], "Rare");
    EXPECT_EQ(pack[1], "Common");
    EXPECT_EQ(pack[8], "Common");
}

// --- openPack rejects layouts that don't fit (design doc Section 7) ---

TEST(PackOpener, RejectsPackWithNoCards)
{
    odds::PackOpener opener{kSeed};
    odds::PackConfig config = packWith(0, {});
    config.totalCardsPerPack = 0;

    EXPECT_THROW(opener.openPack(config), odds::ConfigError);
}

TEST(PackOpener, RejectsSlotCountThatDoesNotMatchPackSize)
{
    odds::PackOpener opener{kSeed};

    // 7 filler + 1 guarantee = 8, but the pack has 9 cards
    EXPECT_THROW(opener.openPack(packWith(7, {certainSlot(9, "Rare")})), odds::ConfigError);
}

TEST(PackOpener, RejectsSlotIndexOutsideThePack)
{
    odds::PackOpener opener{kSeed};

    for (const int bad : {0, -1, 10}) {
        SCOPED_TRACE(bad);
        EXPECT_THROW(opener.openPack(packWith(8, {certainSlot(bad, "Rare")})), odds::ConfigError);
    }
}

TEST(PackOpener, RejectsSameSlotIndexTwice)
{
    odds::PackOpener opener{kSeed};

    EXPECT_THROW(opener.openPack(packWith(7, {certainSlot(9, "Rare"), certainSlot(9, "Super Rare")})),
                 odds::ConfigError);
}
