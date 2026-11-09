#pragma once

#include "odds/PackConfig.h"

#include <cstdint>
#include <random>
#include <string>
#include <vector>

namespace ygo::odds {

// Rolls rarities for pack slots
// Owns random number generator, so create 1 PackOpener per session and reuse it:
// Reseeding per pack can give correlated results on rapid "open all" clicks (design doc Section 7)
class PackOpener {
public:
    // Seeds from std::random_device, so every run gives different results
    // Constructor
    PackOpener();

    // Fixed seed: same seed gives same sequence of rolls, for reproducible runs
    // Constructor
    explicit PackOpener(std::uint32_t seed);

    // Picks one rarity from slot's table with a single uniform roll in [0, 1),
    // walking the cumulative probabilities in table order
    // Throws std::invalid_argument if table is empty
    // Method declaration
    std::string rollSlot(const SlotTable& slot);

    // Opens one pack and returns its cards' rarities in pack order
    // Each guarantee slot is rolled at its slotIndex; every other position is a filler card
    // Throws ConfigError if the config's slots don't fit the pack
    // Method declaration
    std::vector<std::string> openPack(const PackConfig& config);

private:
    // Data member
    std::mt19937 m_rng;
};

} // namespace ygo::odds
