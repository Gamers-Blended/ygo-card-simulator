#include "odds/PackLayout.h"

#include "odds/ConfigError.h"

#include <format>
#include <vector>

namespace ygo::odds {

void validatePackLayout(const PackConfig& config)
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

} // namespace ygo::odds
