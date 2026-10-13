// Console demo for the odds engine. No Qt, no UI: it just prints what the engine sees.
// For now it builds the LOB config by hand; later it will load data/LOB.json and open packs.

#include "odds/PackConfig.h"

#include <iostream>

int main()
{
    // Designated initializers (C++20): each field is named, so this reads like the JSON.
    const ygo::odds::PackConfig lob{
        .setCode = "LOB",
        .totalCardsPerPack = 9,
        .fillerSlots = 8,
        .fillerPool = "Common",
        .guaranteeSlots = {
            {
                .slotIndex = 9,
                .entries = {
                    {.rarity = "Secret Rare", .probability = 1.0 / 36},
                    {.rarity = "Ultra Rare", .probability = 1.0 / 12},
                    {.rarity = "Super Rare", .probability = 1.0 / 6},
                    {.rarity = "Rare", .probability = 26.0 / 36},
                },
            },
        },
        .foilChancePerSlot = 0.05,
    };

    std::cout << "Set " << lob.setCode << ": " << lob.totalCardsPerPack << " cards per pack, "
              << lob.fillerSlots << " filler (" << lob.fillerPool << ")\n";

    for (const auto& slot : lob.guaranteeSlots) {
        std::cout << "  Slot " << slot.slotIndex << ":\n";
        for (const auto& entry : slot.entries) {
            std::cout << "    " << entry.rarity << ": " << entry.probability * 100 << "%\n";
        }
    }

    return 0;
}
