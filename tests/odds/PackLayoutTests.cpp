// Tests for validatePackLayout(): do a pack's slots fit together? (design doc Section 7)

#include "odds/ConfigError.h"
#include "odds/PackLayout.h"

#include <gtest/gtest.h>

#include <string>
#include <utility>
#include <vector>

namespace odds = ygo::odds;

namespace {

// Only the position matters for layout, so the slot's rarity table is left empty
odds::SlotTable slotAt(int slotIndex)
{
    return {.slotIndex = slotIndex};
}

// A 9-card pack shaped like LOB, with the given filler count and guarantee slots
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

TEST(PackLayout, AcceptsLobShapedPack)
{
    EXPECT_NO_THROW(odds::validatePackLayout(packWith(8, {slotAt(9)})));
}

TEST(PackLayout, AcceptsSeveralGuaranteeSlots)
{
    EXPECT_NO_THROW(odds::validatePackLayout(packWith(6, {slotAt(1), slotAt(5), slotAt(9)})));
}

TEST(PackLayout, RejectsPackWithNoCards)
{
    odds::PackConfig config = packWith(0, {});
    config.totalCardsPerPack = 0;

    EXPECT_THROW(odds::validatePackLayout(config), odds::ConfigError);
}

TEST(PackLayout, RejectsSlotCountThatDoesNotMatchPackSize)
{
    // 7 filler + 1 guarantee = 8, but the pack has 9 cards
    EXPECT_THROW(odds::validatePackLayout(packWith(7, {slotAt(9)})), odds::ConfigError);
}

TEST(PackLayout, RejectsSlotIndexOutsideThePack)
{
    for (const int bad : {0, -1, 10}) {
        SCOPED_TRACE(bad);
        EXPECT_THROW(odds::validatePackLayout(packWith(8, {slotAt(bad)})), odds::ConfigError);
    }
}

TEST(PackLayout, RejectsSameSlotIndexTwice)
{
    EXPECT_THROW(odds::validatePackLayout(packWith(7, {slotAt(9), slotAt(9)})), odds::ConfigError);
}

TEST(PackLayout, ErrorMessageNamesTheSetAndSlot)
{
    try {
        odds::validatePackLayout(packWith(8, {slotAt(10)}));
        FAIL() << "expected ConfigError";
    } catch (const odds::ConfigError& error) {
        const std::string errorMessage = error.what();
        EXPECT_NE(errorMessage.find("TEST"), std::string::npos) << errorMessage;
        EXPECT_NE(errorMessage.find("slot 10"), std::string::npos) << errorMessage;
    }
}
