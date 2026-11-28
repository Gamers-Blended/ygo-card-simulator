#include "odds/PackOpener.h"

#include "odds/PackLayout.h"

#include <stdexcept>

namespace ygo::odds {

PackOpener::PackOpener()
    : m_rng(std::random_device{}())
{
}

PackOpener::PackOpener(std::uint32_t seed)
    : m_rng(seed)
{
}

std::string PackOpener::rollSlot(const SlotTable& slot)
{
    if (slot.entries.empty()) {
        throw std::invalid_argument("can't roll an empty slot table");
    }

    // Turns the generator's raw 32-bit numbers into a double in [0, 1)
    std::uniform_real_distribution<double> unit(0.0, 1.0);
    const double roll = unit(m_rng);

    // E.g. LOB: Secret covers [0, 0.028), Ultra [0.028, 0.111), Super [0.111, 0.278), Rare the rest
    double cumulative = 0.0;
    for (const auto& entry : slot.entries) {
        cumulative += entry.probability;
        if (roll < cumulative) {
            return entry.rarity;
        }
    }

    // Only reached if rounding left the total a hair under 1 and the roll landed in that gap
    return slot.entries.back().rarity; // Get rarity of late entry
}

std::vector<std::string> PackOpener::openPack(const PackConfig& config)
{
    // Loaded configs were already checked, but one from built-in code may not have been
    // A bad slotIndex below would write outside the vector (undefined behaviour)
    validatePackLayout(config);

    // Start with every position as filler (parentheses: total copies of fillerPool)
    std::vector<std::string> pack(config.totalCardsPerPack, config.fillerPool);

    // Then overwrite the guarantee positions; slotIndex is 1-based, vector positions are 0-based
    // LOB: [C][C][C][C][C][C][C][C][C] -> slot 9 rolls -> [C][C][C][C][C][C][C][C][R]
    for (const auto& slot : config.guaranteeSlots) {
        pack[slot.slotIndex - 1] = rollSlot(slot);
    }

    return pack;
}

} // namespace ygo::odds
