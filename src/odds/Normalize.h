#pragma once

#include "odds/PackConfig.h"
#include "odds/RawSlotTable.h"

namespace ygo::odds {

// Converts a slot as written in config into plain probabilities (design doc Section 7):
// RawSlotTable -> SlotTable
//   Weight    -> weight / sum of all weights in table
//   OneInN    -> 1 / N
//   Remainder -> 1 - sum of all OneInN probabilities
// Entries keep their order, since roll walks them in order
// Doesn't validate yet: a bad table (e.g. N = 0) gives a bad result
SlotTable normalize(const RawSlotTable& raw);

} // namespace ygo::odds
