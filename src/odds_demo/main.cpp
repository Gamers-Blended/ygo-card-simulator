// Console demo for odds engine
// No Qt, no UI: just prints what engine sees
// For now it builds LOB config by hand; later it will load data/LOB.json and open packs

#include "odds/PackConfig.h"
#include "odds/RawSlotTable.h"

#include <iostream>

namespace {

// Prints how an entry's odds were written in the config, e.g. "1 in 36"
void printRawOdds(const ygo::odds::RawRarityEntry& entry)
{
    using ygo::odds::OddsKind;

    switch (entry.kind) {
    case OddsKind::Weight:
        std::cout << "weight " << entry.value;
        break;
    case OddsKind::OneInN:
        std::cout << "1 in " << entry.value;
        break;
    case OddsKind::Remainder:
        std::cout << "remainder";
        break;
    }
}

} // namespace

int main()
{
    using ygo::odds::OddsKind;

    // LOB's guarantee slot as the config file writes it (design doc Section 7)
    const ygo::odds::RawSlotTable lobRawSlot{
        .slotIndex = 9,
        .entries = {
            {.rarity = "Secret Rare", .kind = OddsKind::OneInN, .value = 36},
            {.rarity = "Ultra Rare", .kind = OddsKind::OneInN, .value = 12},
            {.rarity = "Super Rare", .kind = OddsKind::OneInN, .value = 6},
            {.rarity = "Rare", .kind = OddsKind::Remainder},
        },
    };

    std::cout << "Raw slot " << lobRawSlot.slotIndex << " (as written in the config):\n";
    for (const auto& entry : lobRawSlot.entries) {
        std::cout << "    " << entry.rarity << ": ";
        printRawOdds(entry);
        std::cout << '\n';
    }

    // Designated initializers (C++20): each field is named, so this reads like the JSON
    // The probabilities are converted by hand for now; step 2 will compute them from lobRawSlot
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
    };

    std::cout << "\nSet " << lob.setCode << ": " << lob.totalCardsPerPack << " cards per pack, "
              << lob.fillerSlots << " filler (" << lob.fillerPool << ")\n";

    for (const auto& slot : lob.guaranteeSlots) {
        std::cout << "  Slot " << slot.slotIndex << " (as the engine sees it):\n";
        for (const auto& entry : slot.entries) {
            std::cout << "    " << entry.rarity << ": " << entry.probability * 100 << "%\n";
        }
    }

    return 0;
}
