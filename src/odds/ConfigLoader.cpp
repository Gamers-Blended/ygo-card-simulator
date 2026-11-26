#include "odds/ConfigLoader.h"

#include "odds/ConfigError.h"
#include "odds/Normalize.h"
#include "odds/RawSlotTable.h"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <format>
#include <string>

namespace ygo::odds {

namespace {

using json = nlohmann::json;

// Every helper takes `where`, the path to the JSON value being read (e.g. "guarantee_slots[0]"),
// so an error message says where in the file the problem is

void requireObject(const json& value, std::string_view where)
{
    if (!value.is_object()) {
        throw ConfigError(std::format("{}: must be an object (got {})", where, value.type_name()));
    }
}

// Returns the field's value without copying it: the reference points into `object`,
// which the caller keeps alive for as long as it uses the result
const json& requireField(const json& object, std::string_view key, std::string_view where)
{
    if (!object.contains(key)) {
        throw ConfigError(std::format("{}: missing field \"{}\"", where, key));
    }
    return object.at(key);
}

// Every string in the schema is a name (set code, rarity...), so an empty one is an error too
std::string getString(const json& object, std::string_view key, std::string_view where)
{
    const json& value = requireField(object, key, where);
    if (!value.is_string()) {
        throw ConfigError(std::format("{}: \"{}\" must be a string (got {})", where, key, value.type_name()));
    }
    auto text = value.get<std::string>();
    if (text.empty()) {
        throw ConfigError(std::format("{}: \"{}\" must not be empty", where, key));
    }
    return text;
}

// Whole numbers only: 9.5 cards per pack makes no sense, so a JSON float is rejected
int getInt(const json& object, std::string_view key, std::string_view where)
{
    const json& value = requireField(object, key, where);
    if (!value.is_number_integer()) {
        throw ConfigError(std::format("{}: \"{}\" must be a whole number (got {})", where, key, value.type_name()));
    }
    return value.get<int>();
}

// Any JSON number, whole or not ("1 in 2.5" is fine)
double getNumber(const json& object, std::string_view key, std::string_view where)
{
    const json& value = requireField(object, key, where);
    if (!value.is_number()) {
        throw ConfigError(std::format("{}: \"{}\" must be a number (got {})", where, key, value.type_name()));
    }
    return value.get<double>();
}

bool getBool(const json& object, std::string_view key, std::string_view where)
{
    const json& value = requireField(object, key, where);
    if (!value.is_boolean()) {
        throw ConfigError(std::format("{}: \"{}\" must be true or false (got {})", where, key, value.type_name()));
    }
    return value.get<bool>();
}

const json& getArray(const json& object, std::string_view key, std::string_view where)
{
    const json& value = requireField(object, key, where);
    if (!value.is_array()) {
        throw ConfigError(std::format("{}: \"{}\" must be an array (got {})", where, key, value.type_name()));
    }
    return value;
}

// One table entry, e.g. { "rarity": "Super Rare", "odds_denominator": 6 }
RawRarityEntry parseEntry(const json& entry, std::string_view where)
{
    requireObject(entry, where);

    RawRarityEntry raw{.rarity = getString(entry, "rarity", where)};

    // Exactly one odds key, otherwise it's unclear which one was meant
    int oddsKeyCount = 0;
    for (const std::string_view key : {"odds_denominator", "remainder"}) {
        if (entry.contains(key)) {
            ++oddsKeyCount;
        }
    }
    if (oddsKeyCount != 1) {
        throw ConfigError(std::format("{}: needs exactly one of \"odds_denominator\" or \"remainder\" (got {})",
                                      where, oddsKeyCount));
    }

    // Range check (N >= 1) is left to normalize(), so each rule lives in one place
    if (entry.contains("odds_denominator")) {
        raw.kind = OddsKind::OneInN;
        raw.value = getNumber(entry, "odds_denominator", where);
    } else {
        // "remainder": false would read as "not the remainder", which leaves the entry with no odds at all
        if (!getBool(entry, "remainder", where)) {
            throw ConfigError(std::format("{}: \"remainder\" must be true if present", where));
        }
        raw.kind = OddsKind::Remainder;
    }
    return raw;
}

// One guarantee slot: { "slot_index": 9, "table": [ ... ] }
RawSlotTable parseSlot(const json& slot, std::string_view where)
{
    requireObject(slot, where);

    RawSlotTable raw{.slotIndex = getInt(slot, "slot_index", where)};

    const json& table = getArray(slot, "table", where);
    raw.entries.reserve(table.size());
    for (std::size_t i = 0; i < table.size(); ++i) {
        raw.entries.push_back(parseEntry(table[i], std::format("{}.table[{}]", where, i)));
    }
    return raw;
}

} // namespace

PackConfig parsePackConfig(std::string_view jsonText)
{
    json root;
    try {
        root = json::parse(jsonText);
    } catch (const json::parse_error& error) {
        // nlohmann's message already gives the line and column
        throw ConfigError(std::format("not valid JSON: {}", error.what()));
    }

    constexpr std::string_view kTop = "pack config";
    requireObject(root, kTop);

    // Members of a braced initializer are evaluated in order, so the first bad field is the one reported
    PackConfig config{
        .setCode = getString(root, "set_code", kTop),
        .totalCardsPerPack = getInt(root, "total_cards_per_pack", kTop),
        .fillerSlots = getInt(root, "filler_slots", kTop),
        .fillerPool = getString(root, "filler_pool", kTop),
    };

    const json& slots = getArray(root, "guarantee_slots", kTop);
    config.guaranteeSlots.reserve(slots.size());
    for (std::size_t i = 0; i < slots.size(); ++i) {
        // Parse to the raw form first, then normalize, the same path a hand-built table takes
        const RawSlotTable raw = parseSlot(slots[i], std::format("guarantee_slots[{}]", i));
        config.guaranteeSlots.push_back(normalize(raw));
    }
    return config;
}

} // namespace ygo::odds
