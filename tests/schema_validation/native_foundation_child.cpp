// SPDX-License-Identifier: GPL-3.0-or-later
// Fixed nonvalidating exec probe. No Python, SDK, owner, protocol or GTest.
#include "native_foundation_child.h"
#include <climits>

namespace {
bool literal(const char *value, const char *expected) noexcept {
    for (unsigned i = 0; i < 32; ++i) {
        if (value[i] != expected[i])
            return false;
        if (expected[i] == '\0')
            return true;
    }
    return false;
}
int descriptor(const char *value) noexcept {
    unsigned result = 0;
    for (unsigned i = 0; i <= 10; ++i) {
        const unsigned byte = static_cast<unsigned char>(value[i]);
        if (byte == 0)
            return i && result <= INT_MAX ? static_cast<int>(result) : -1;
        if (byte < '0' || byte > '9' || result > (INT_MAX - (byte - '0')) / 10U)
            return -1;
        result = result * 10 + byte - '0';
    }
    return -1;
}
} // namespace

int main(int argc, char **argv) {
    using namespace schema_foundation;
    if (argc != 5 || !literal(argv[1], "--facility-observation"))
        finish(115);
    const int channel = descriptor(argv[2]), hold_read = descriptor(argv[3]),
              extra = descriptor(argv[4]);
    if (channel < 0 || hold_read < 0 || extra < 0)
        finish(116);
    Report report;
    observe(report, 2, channel, extra);
    publish(report, channel);
    hold(hold_read); // The inherited five-second kill-only timer remains active.
}
