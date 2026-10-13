#pragma once

#include <string>
#include <vector>

namespace ygo::odds {

// One possible result of a slot, e.g. "Super Rare" with probability 1/6.
// probability is always the final chance in [0, 1]. Configs written as weights or
// "1 in N" odds are converted to this form when they're loaded, so the rest of the
// engine only ever deals with plain probabilities.
struct RarityEntry {
    std::string rarity;
    double probability = 0.0;
};

// A pack slot that rolls on its own rarity table instead of drawing from the filler pool.
struct SlotTable {
    int slotIndex = 0; // 1-based position in the pack, same as "slot_index" in the JSON
    std::vector<RarityEntry> entries;
};

// Everything the engine needs to know to open one pack of a set.
// Mirrors the per-set JSON schema in design doc Section 7.
struct PackConfig {
    std::string setCode;
    int totalCardsPerPack = 0;
    int fillerSlots = 0;
    std::string fillerPool;
    std::vector<SlotTable> guaranteeSlots;
    double foilChancePerSlot = 0.0;
};

} // namespace ygo::odds
