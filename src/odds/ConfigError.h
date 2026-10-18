#pragma once

#include <stdexcept>

namespace ygo::odds {

// Thrown when a set's odds config is invalid (design doc Section 7: reject bad configs at load time)
// Deriving from std::runtime_error gives it what() for the message, and lets code that only
// cares about "something failed" catch std::exception instead
class ConfigError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error; // reuse runtime_error's constructor that takes message
};

} // namespace ygo::odds
