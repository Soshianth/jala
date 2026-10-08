// =============================================================================
// gregorian.cpp — minimal Gregorian calendar implementation
// SPDX-License-Identifier: MIT
// =============================================================================

#include "gregorian.hpp"

#include <ctime>

namespace jala {

GregorianDate today() noexcept {
    const std::time_t t  = std::time(nullptr);
    const std::tm*    lt = std::localtime(&t);

    // localtime may return nullptr on some platforms if the calendar
    // time cannot be represented. Fall back to the Unix epoch so the
    // caller always receives a usable date.
    if (lt == nullptr) {
        return { 1970, 1, 1 };
    }
    return { lt->tm_year + 1900, lt->tm_mon + 1, lt->tm_mday };
}

}  // namespace jala