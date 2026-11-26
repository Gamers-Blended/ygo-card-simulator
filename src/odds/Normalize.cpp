#include "odds/Normalize.h"

#include "odds/ConfigError.h"

#include <algorithm>
#include <cmath>
#include <format>
#include <string>

namespace ygo::odds {

namespace {

// 1-in-N totals a tiny bit over 1 still count as 1, since doubles can't hold most fractions
// exactly (e.g. 1/2 + 1/12 + 1/3 + 1/20 + 1/30 is exactly 1, but adds up to 1.0000000000000002)
constexpr double kSumTolerance = 1e-9;

// Builds error for a bad table, prefixed with slot so message points into config
ConfigError slotError(const RawSlotTable& raw, const std::string& message)
{
    return ConfigError(std::format("slot {}: {}", raw.slotIndex, message));
}

} // namespace

SlotTable normalize(const RawSlotTable& raw)
{
    if (raw.entries.empty()) {
        throw slotError(raw, "table has no entries");
    }

    // Pass 1: check each entry, and add up the 1-in-N chances, because the Remainder entry's
    // chance depends on the whole table
    double oneInNSum = 0.0;
    int remainderCount = 0;

    for (const auto& entry : raw.entries) {
        if (entry.rarity.empty()) {
            throw slotError(raw, "entry has no rarity name");
        }

        switch (entry.kind) {
        case OddsKind::OneInN:
            // N < 1 would mean a chance above 100%
            // isfinite rejects NaN and infinity, which would poison the sum
            if (!std::isfinite(entry.value) || entry.value < 1.0) {
                throw slotError(raw, std::format("{}: 1 in N needs N >= 1 (got {})", entry.rarity,
                                                 entry.value));
            }
            oneInNSum += 1.0 / entry.value;
            break;
        case OddsKind::Remainder:
            ++remainderCount;
            break;
        }
    }

    // Table-wide rules, now that every entry has been seen
    // Without exactly one remainder, the table's chances wouldn't add up to 1
    if (remainderCount != 1) {
        throw slotError(raw, std::format("table needs exactly one remainder entry (got {})", remainderCount));
    }
    if (oneInNSum > 1.0 + kSumTolerance) {
        throw slotError(raw, std::format("1-in-N odds add up to {}, more than 1", oneInNSum));
    }

    // Pass 2: get each entry's probability, now that the totals are known and the table is valid
    SlotTable result{.slotIndex = raw.slotIndex};
    result.entries.reserve(raw.entries.size());

    for (const auto& entry : raw.entries) {
        double probability = 0.0;
        switch (entry.kind) {
        case OddsKind::OneInN:
            probability = 1.0 / entry.value;
            break;
        case OddsKind::Remainder:
            // max() turns a tiny negative from rounding (sum within tolerance of 1) into 0
            probability = std::max(0.0, 1.0 - oneInNSum);
            break;
        }
        result.entries.push_back({.rarity = entry.rarity, .probability = probability});
    }

    return result;
}

} // namespace ygo::odds
