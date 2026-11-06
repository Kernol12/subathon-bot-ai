// SPDX-License-Identifier: GPL-3.0-or-later
#include "transport_v1.h"

// Explicit inert native baseline; no argument inspection or resource/worker operation.
extern "C" int subathon_schema_transport_v1(const unsigned char *, unsigned int,
                                            const unsigned char *, unsigned int,
                                            const unsigned char *, unsigned int,
                                            const unsigned char *, unsigned int) noexcept {
    return 31;
}
