#pragma once

#include "odds/PackConfig.h"

#include <string_view>

// Turns a per-set config (data/*.json, schema in design doc Section 7) into a PackConfig
// No JSON types appear here: nlohmann/json is only used inside ConfigLoader.cpp,
// so code that includes this header doesn't need the JSON library at all

namespace ygo::odds {

// Parses config text and normalizes every guarantee slot, so the result is ready to roll on
// Each table entry gives its odds with exactly one of:
//   "weight": number | "odds_denominator": number | "remainder": true
//
// Throws ConfigError if:
// - text isn't valid JSON
// - a field is missing, has the wrong type, or is an empty string
// - a table entry has zero or several odds keys, or "remainder" isn't true
// - a table breaks a normalize() rule (see Normalize.h)
// Messages name the place in the file, e.g. "guarantee_slots[0].table[2]: ..."
PackConfig parsePackConfig(std::string_view jsonText);

} // namespace ygo::odds
