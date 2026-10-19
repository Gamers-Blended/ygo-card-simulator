#include "odds/PackOpener.h"

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
    return slot.entries.back().rarity;
}

} // namespace ygo::odds
