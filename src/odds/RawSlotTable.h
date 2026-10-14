#pragma once

#include <string>
#include <vector>

namespace ygo::odds {

// How a config file writes the chance of one rarity
enum class OddsKind {
    Weight,    // relative weight: chance = weight / sum of all weights in the table
    OneInN,    // advertised "1 in N packs": chance = 1 / N
    Remainder, // whatever chance is left over after the OneInN entries
};

// One table entry exactly as written in the config, before it's turned into a probability
struct RawRarityEntry {
    std::string rarity;
    OddsKind kind = OddsKind::Weight;
    double value = 0.0; // the weight, or N for OneInN; unused for Remainder
};

// A guarantee slot as written in the config
// Normalizing it (design doc Section 7) produces a SlotTable from PackConfig.h
struct RawSlotTable {
    int slotIndex = 0;
    std::vector<RawRarityEntry> entries;
};

} // namespace ygo::odds
