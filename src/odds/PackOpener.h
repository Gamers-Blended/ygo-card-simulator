#pragma once

#include "odds/PackConfig.h"

#include <cstdint>
#include <random>
#include <string>

namespace ygo::odds {

// Rolls rarities for pack slots
// Owns the random number generator, so create one PackOpener per session and reuse it:
// reseeding per pack can give correlated results on rapid "open all" clicks (design doc Section 7)
class PackOpener {
public:
    // Seeds from std::random_device, so every run gives different results
    PackOpener();

    // Fixed seed: the same seed gives the same sequence of rolls, for reproducible runs
    explicit PackOpener(std::uint32_t seed);

    // Picks one rarity from the slot's table with a single uniform roll in [0, 1),
    // walking the cumulative probabilities in table order
    // Throws std::invalid_argument if the table is empty
    std::string rollSlot(const SlotTable& slot);

private:
    std::mt19937 m_rng;
};

} // namespace ygo::odds
