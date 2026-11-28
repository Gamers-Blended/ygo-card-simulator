// Console demo for odds engine
// No Qt, no UI: just prints what engine sees
// Loads a set's config from path given on the command line
// e.g.
// ./build/cli/odds_demo data/LOB.json

#include "odds/ConfigError.h"
#include "odds/ConfigLoader.h"
#include "odds/PackConfig.h"
#include "odds/PackOpener.h"

#include <cstdlib>
#include <filesystem>
#include <format>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// Alias for engine's namespace, only in this file (never put in a header)
namespace odds = ygo::odds;

namespace {

// Prints a slot's probabilities, plus each as "1 in N" so it can be checked against printed odds
void printSlot(const odds::SlotTable& slot)
{
    std::cout << "  Slot " << slot.slotIndex << ":\n";
    for (const auto& entry : slot.entries) {
        std::cout << std::format("    {}: {:.2f}%", entry.rarity, entry.probability * 100);
        // A remainder can come out as exactly 0; 1 / 0 would print "inf"
        if (entry.probability > 0.0) {
            std::cout << std::format(" (1 in {:.2f})", 1.0 / entry.probability);
        }
        std::cout << '\n';
    }
}

// Everything the demo shows for one loaded set
void runDemo(const odds::PackConfig& config)
{
    std::cout << "Set " << config.setCode << ": " << config.totalCardsPerPack << " cards per pack, "
              << config.fillerSlots << " filler (" << config.fillerPool << ")\n";
    for (const auto& slot : config.guaranteeSlots) {
        printSlot(slot);
    }

    // Roll each guarantee slot many times and compare how often each rarity came up with its odds
    // One PackOpener for the whole run: it's seeded once, never per roll
    odds::PackOpener opener;
    constexpr int kRolls = 36000;

    for (const auto& slot : config.guaranteeSlots) {
        std::map<std::string, int> counts; // rarity -> how many times it was rolled
        for (int i = 0; i < kRolls; ++i) {
            ++counts[opener.rollSlot(slot)];
        }

        std::cout << "\nRolled slot " << slot.slotIndex << " " << kRolls << " times:\n";
        for (const auto& entry : slot.entries) {
            const int count = counts[entry.rarity];
            const double observed = 100.0 * count / kRolls;
            std::cout << std::format("    {}: {} ({:.2f}%, expected {:.2f}%)\n", entry.rarity, count, observed,
                                     entry.probability * 100);
        }
    }

    // Open whole packs with the same opener, so the RNG carries on from the rolls above
    constexpr int kPacks = 5;

    std::cout << "\nOpening " << kPacks << " " << config.setCode << " packs:\n";
    for (int i = 1; i <= kPacks; ++i) {
        const std::vector<std::string> pack = opener.openPack(config);

        std::cout << "  Pack " << i << ":";
        for (const auto& rarity : pack) {
            std::cout << " [" << rarity << "]";
        }
        std::cout << '\n';
    }
}

} // namespace

// argc = how many command-line words there are, counting the program's own name
// argv = those words as C strings: argv[0] is the program, argv[1] the first argument, ...
int main(int argc, char* argv[])
{
    if (argc != 2) {
        // argv[0] can be missing (argc == 0) if the program was started in an unusual way
        const char* programName = argc > 0 ? argv[0] : "odds_demo";
        // Errors go to std::cerr, not std::cout, so they still show if normal output is redirected to a file
        std::cerr << "Usage: " << programName << " <set config, e.g. data/LOB.json>\n";
        return EXIT_FAILURE;
    }

    const std::filesystem::path configPath = argv[1];

    // A bad config is the user's mistake, not a bug: report it and exit cleanly instead of crashing
    try {
        runDemo(odds::loadPackConfig(configPath));
    } catch (const odds::ConfigError& error) {
        std::cerr << "Error: " << error.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
