// SPDX-License-Identifier: GPL-3.0-or-later
#include "sqlite_v22_catalog.hpp"

namespace subathon::server::detail {
namespace {

// Sealed against the independent STORAGE-003 catalog oracle at c8aa2702.
constexpr V22CatalogIdentity identity{
    1,
    "subathon.sqlite-store",
    {{
        {"retain-config-context-v1",
         "0906f121c3ba1193cdea41a70f1c91013724ea90416b0f1d04b6c23d8f091f5e"},
        {"identified-receipts-v1",
         "24614f68e9c41a29751ea9ef6ba08fcf655b1cdb0e0c097cb1601bb1581553a2"},
        {"contribution-evidence-v1",
         "b72249ba22c4838af98b9ae29600e70969cf3295eea958a30074167221d2ad1d"},
        {"captured-application-plans-v1",
         "bbb8140119b29f54f419479851d4fdf42b6a928f5451e4de756389d2bc359c50"},
        {"atomic-application-bundles-v1",
         "c9ba98fa4fe2a42cc6ed9776baa011fc2b93cf879e7374b53e9a6128f7d7afbb"},
        {"durable-application-holds-v1",
         "9f5b61d41dc644c417808d781cea73144a8d019ad95646373c30751e71596d0a"},
        {"reference-initial-captures-v1",
         "efe4d5927002c1849ee51b9c3a988ea5b8bcab9eb8547ad5f12387b27c33b218"},
        {"account-sessions-v1",
         "ec41d4b3bf7d04a3e846b401060cc3ed9430020db3dfce0e2650ed76f6a46d5b"},
        {"permission-graph-v1",
         "a818324e2ff37a638fa00dcb89e3c4b89140fe7e09e1c6eb06d20bcc38be7f68"},
        {"reference-authority-v1",
         "d0c8f1348abbc3521293ee2d3af3fa75fbeb65c40e5a5589c3c1e9953e172315"},
        {"delivery-jobs-v1",
         "342ab385bd4b70d75ec5bbf16ef0b34d6173d5a4f41da539a85d1c6412cdfbc7"},
        {"milestone-groups-v1",
         "590881656952aa89350784c4de3ea50a7d002c3d1b00baf21f1e04aede36f18e"},
        {"milestone-group-decisions-v1",
         "06479f8418119285626522c9f4fa84096b28a594c617a017c8b842ddbd37f7de"},
        {"modifier-activations-v1",
         "a6ca951e91632dd7a2f5bb0f4028a95eb6b0397545cf942a8e34d690e65ea1c8"},
        {"financial-refunds-v1",
         "93a31cfe433644a09ce603bc211390950e481b1e0bdef8d36e579160e7d2c7b6"},
        {"provider-credentials-v1",
         "7246930c294455e1d6542dccaaa0d7f3727fd6c2444cf29f8a4f8b32d2ca6b3d"},
        {"rate-overrides-v1",
         "b14cfdd3a3f69b84d884c4a274d9f4d14389b958726ad5b500e648fddac6783f"},
        {"milestone-target-edits-v1",
         "1cc2095026cf3d296c11ceb05a74b56e5228951161acc64834c9a00cc17646f4"},
        {"contribution-review-v1",
         "407229e831e3f1368f2b5cba415c56fe0cc1b45a30ddfb496c39ec72504e8e87"},
        {"connector-intake-storage-v1",
         "eb1758529cd052df4f8ed80ab693a6219afa519ae9db5f1b00f2f6bae05bf7e1"},
        {"milestone-slot-interruption-storage-v1",
         "52b68a611a96311f137c033cf5b0a376a585e555a56b555e20de3938d4db463e"},
        {"clock-checkpoints-v1",
         "2c8b634d2ed28a81f949e8a31e7d9de991bf513f4f5000194b1eaafb725d27bf"},
    }},
    292,
    "382a5e86e3b6f4b2e2b8a1f3aa73f7e8f20f3e24413a7fb50d6f5a7ea6f8e3ff",
};

} // namespace

const V22CatalogIdentity &sqlite_v22_catalog() noexcept { return identity; }

} // namespace subathon::server::detail
