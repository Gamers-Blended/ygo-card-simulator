#include "odds/PackOpener.h"

#include "odds/ConfigError.h"

#include <format>
#include <stdexcept>

namespace ygo::odds {

namespace {

// Checks that slots fit pack, so openPack can safely index by slotIndex
void checkPackLayout(const PackConfig& config)
{
    const int total = config.totalCardsPerPack;
    if (total <= 0) {
        throw ConfigError(std::format("{}: pack needs at least 1 card (got {})", config.setCode, total));
    }

    const int guaranteeCount = static_cast<int>(config.guaranteeSlots.size());
    if (config.fillerSlots + guaranteeCount != total) {
        throw ConfigError(std::format("{}: {} filler + {} guarantee slots doesn't match {} cards per pack",
                                      config.setCode, config.fillerSlots, guaranteeCount, total));
    }

    // How many guarantee slots claim each position (parentheses: total elements, all 0)
    std::vector<int> claims(total, 0);
    for (const auto& slot : config.guaranteeSlots) {
        if (slot.slotIndex < 1 || slot.slotIndex > total) {
            throw ConfigError(std::format("{}: slot {} is outside the pack (1 to {})", config.setCode,
                                          slot.slotIndex, total));
        }
        if (++claims[slot.slotIndex - 1] > 1) {
            throw ConfigError(std::format("{}: slot {} is defined twice", config.setCode, slot.slotIndex));
        }
    }
}

} // namespace

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
    checkPackLayout(config);

    // Start with every position as filler (parentheses: total copies of fillerPool)
    std::vector<std::string> pack(config.totalCardsPerPack, config.fillerPool);

    // Then overwrite the guarantee positions; slotIndex is 1-based, vector positions are 0-based
    for (const auto& slot : config.guaranteeSlots) {
        pack[slot.slotIndex - 1] = rollSlot(slot);
    }

    return pack;
}

} // namespace ygo::odds
