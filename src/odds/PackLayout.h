#pragma once

#include "odds/PackConfig.h"

namespace ygo::odds {

// Checks that a pack's slots fit together (design doc Section 7), so openPack can safely
// put each guarantee slot at position slotIndex
// Called when a config is loaded, and again by openPack for configs built in code
//
// Throws ConfigError if:
// - the pack has no cards
// - filler slots + guarantee slots doesn't equal the cards per pack
// - a guarantee slot's index is outside 1..cards per pack
// - two guarantee slots use the same index
void validatePackLayout(const PackConfig& config);

} // namespace ygo::odds
