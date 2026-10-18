#include "odds/Normalize.h"

namespace ygo::odds {

SlotTable normalize(const RawSlotTable& raw)
{
    // Pass 1: get sum/total of each kind, because a Weight or Remainder entry's chance depends on whole table
    double weightSum = 0.0;
    double oneInNSum = 0.0;
    for (const auto& entry : raw.entries) {
        switch (entry.kind) {
        case OddsKind::Weight:
            weightSum += entry.value;
            break;
        case OddsKind::OneInN:
            oneInNSum += 1.0 / entry.value;
            break;
        case OddsKind::Remainder:
            break;
        }
    }

    // Pass 2: get each entry's probability, now that the totals are known
    SlotTable result{.slotIndex = raw.slotIndex};
    result.entries.reserve(raw.entries.size());

    for (const auto& entry : raw.entries) {
        double probability = 0.0;
        switch (entry.kind) {
        case OddsKind::Weight:
            probability = entry.value / weightSum;
            break;
        case OddsKind::OneInN:
            probability = 1.0 / entry.value;
            break;
        case OddsKind::Remainder:
            probability = 1.0 - oneInNSum;
            break;
        }
        result.entries.push_back({.rarity = entry.rarity, .probability = probability});
    }

    return result;
}

} // namespace ygo::odds
