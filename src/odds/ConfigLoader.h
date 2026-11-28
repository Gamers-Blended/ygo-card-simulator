#pragma once

#include "odds/PackConfig.h"

#include <filesystem>
#include <string_view>

// Turns a per-set config (data/*.json, schema in design doc Section 7) into a PackConfig

namespace ygo::odds {

// Parses config text and normalizes every guarantee slot, so the result is ready to roll on
// Each table entry gives its odds with exactly one of:
//   "odds_denominator": number | "remainder": true
//
// Throws ConfigError if:
// - text isn't valid JSON
// - a field is missing, has the wrong type, or is an empty string
// - a table entry has zero or several odds keys, or "remainder" isn't true
// - a table breaks a normalize() rule (see Normalize.h)
// - the slots don't fit the pack (see validatePackLayout() in PackLayout.h)
// Messages name the place in the file, e.g. "guarantee_slots[0].table[2]: ..."
PackConfig parsePackConfig(std::string_view jsonText);

// Reads a config file and parses it with parsePackConfig()
// Throws ConfigError if the file can't be opened, or for any parsePackConfig() error;
// either way the message starts with the file's path, e.g. "data/LOB.json: missing field ..."
PackConfig loadPackConfig(const std::filesystem::path& path);

} // namespace ygo::odds
