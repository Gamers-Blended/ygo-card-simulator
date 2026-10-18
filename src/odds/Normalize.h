#pragma once

#include "odds/PackConfig.h"
#include "odds/RawSlotTable.h"

namespace ygo::odds {

// Converts a slot as written in config into plain probabilities (design doc Section 7):
// RawSlotTable -> SlotTable
// Weight    -> weight / sum of all weights in table
// OneInN    -> 1 / N
// Remainder -> 1 - sum of all OneInN probabilities
// Entries keep their order, since roll walks them in order
//
// Throws ConfigError if table is invalid:
// - no entries, or an entry with no rarity name
// - a weight that isn't > 0, or a 1-in-N with N < 1
// - weights mixed with 1-in-N / remainder entries
// - a 1-in-N table without exactly one remainder entry
// - 1-in-N odds adding up to more than 1
SlotTable normalize(const RawSlotTable& raw);

} // namespace ygo::odds
