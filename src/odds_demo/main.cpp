// Console demo for odds engine
// No Qt, no UI: just prints what engine sees
// For now it builds LOB config by hand; later it will load data/LOB.json and open packs

#include "odds/ConfigError.h"
#include "odds/Normalize.h"
#include "odds/PackConfig.h"
#include "odds/RawSlotTable.h"

#include <iostream>
#include <vector>

// Alias for engine's namespace, only in this file (never put in a header)
namespace odds = ygo::odds;

namespace {

// Prints how an entry's odds were written in config, e.g. "1 in 36"
// Function reads caller's object instead of copying its string
void printRawOdds(const odds::RawRarityEntry& entry)
{
    switch (entry.kind) {
    case odds::OddsKind::Weight:
        std::cout << "weight " << entry.value;
        break;
    case odds::OddsKind::OneInN:
        std::cout << "1 in " << entry.value;
        break;
    case odds::OddsKind::Remainder:
        std::cout << "remainder";
        break;
    }
}

// Prints a raw slot next to its normalized probabilities, one line per entry
void printSlot(const odds::RawSlotTable& raw)
{
    const odds::SlotTable normalized = odds::normalize(raw);

    std::cout << "  Slot " << raw.slotIndex << ":\n";
    // Same order in both tables, so entry i of one matches entry i of the other
    for (std::size_t i = 0; i < raw.entries.size(); ++i) {
        std::cout << "    " << raw.entries[i].rarity << ": ";
        printRawOdds(raw.entries[i]);
        std::cout << " -> " << normalized.entries[i].probability * 100 << "%\n";
    }
}

} // namespace

int main()
{
    // LOB's guarantee slot as the config file writes it (design doc Section 7)
    // Designated initializers (C++20): each field is named, so this reads like the JSON
    const odds::RawSlotTable lobRawSlot{
        .slotIndex = 9,
        .entries = {
            {.rarity = "Secret Rare", .kind = odds::OddsKind::OneInN, .value = 36},
            {.rarity = "Ultra Rare", .kind = odds::OddsKind::OneInN, .value = 12},
            {.rarity = "Super Rare", .kind = odds::OddsKind::OneInN, .value = 6},
            {.rarity = "Rare", .kind = odds::OddsKind::Remainder},
        },
    };

    const odds::PackConfig lob{
        .setCode = "LOB",
        .totalCardsPerPack = 9,
        .fillerSlots = 8,
        .fillerPool = "Common",
        .guaranteeSlots = {odds::normalize(lobRawSlot)},
    };

    std::cout << "Set " << lob.setCode << ": " << lob.totalCardsPerPack << " cards per pack, "
              << lob.fillerSlots << " filler (" << lob.fillerPool << ")\n";
    printSlot(lobRawSlot);

    // The generic weighted example from design doc Section 7, to exercise the Weight path
    const odds::RawSlotTable weightedSlot{
        .slotIndex = 9,
        .entries = {
            {.rarity = "Rare", .kind = odds::OddsKind::Weight, .value = 70},
            {.rarity = "Super", .kind = odds::OddsKind::Weight, .value = 20},
            {.rarity = "Ultra", .kind = odds::OddsKind::Weight, .value = 10},
        },
    };

    std::cout << "\nWeighted example:\n";
    printSlot(weightedSlot);

    // Tables normalize() must reject, to show validation and exception handling
    const std::vector<odds::RawSlotTable> badSlots{
        {.slotIndex = 9,
         .entries = {
             {.rarity = "Super Rare", .kind = odds::OddsKind::OneInN, .value = 0},
             {.rarity = "Rare", .kind = odds::OddsKind::Remainder},
         }},
        {.slotIndex = 9,
         .entries = {
             {.rarity = "Super Rare", .kind = odds::OddsKind::OneInN, .value = 6},
             {.rarity = "Rare", .kind = odds::OddsKind::Weight, .value = 70},
         }},
        {.slotIndex = 9,
         .entries = {
             {.rarity = "Super Rare", .kind = odds::OddsKind::OneInN, .value = 2},
             {.rarity = "Ultra Rare", .kind = odds::OddsKind::OneInN, .value = 1},
             {.rarity = "Rare", .kind = odds::OddsKind::Remainder},
         }},
        {.slotIndex = 9,
         .entries = {
             {.rarity = "Super Rare", .kind = odds::OddsKind::OneInN, .value = 6},
         }},
    };

    std::cout << "\nInvalid tables:\n";
    for (const auto& bad : badSlots) {
        try {
            odds::normalize(bad);
            std::cout << "  accepted (this is a bug!)\n";
        } catch (const odds::ConfigError& error) {
            std::cout << "  rejected: " << error.what() << '\n';
        }
    }

    return 0;
}
