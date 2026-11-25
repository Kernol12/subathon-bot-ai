// SPDX-License-Identifier: GPL-3.0-or-later
#include "../../../server/src/sqlite_clock_checkpoint.hpp"
#include "../../../server/src/sqlite_v22_catalog.hpp"

#include <QCryptographicHash>
#include <QDebug>

#include <algorithm>

int main() {
    using namespace subathon::server;
    const auto built = detail::clock_checkpoint_manifest();
    const auto *manifest = std::get_if<MigrationManifest>(&built);
    const auto &seal = detail::sqlite_v22_catalog();
    const auto steps = static_cast<qsizetype>(seal.steps.size());
    if (!manifest || manifest->catalog_format != seal.format ||
        manifest->store_identity.toUtf8().toStdString() != seal.store ||
        manifest->steps.size() != steps ||
        manifest->supported_starting_versions.size() != steps + 1 ||
        manifest->supported_starting_versions.front() != 0 ||
        manifest->steps.back().expected_schema.size() != seal.projection_objects) {
        qCritical("Compiled v22 catalog identity or projection size differs from the seal");
        return 1;
    }

    const auto projection = detail::sqlite_v22_projection();
    QList<CatalogObject> expected_schema;
    expected_schema.reserve(static_cast<qsizetype>(projection.size()));
    for (const auto &object : projection) {
        expected_schema.append(
            {QString::fromUtf8(object.type), QString::fromUtf8(object.name),
             QString::fromUtf8(object.table_name),
             object.sql ? std::optional{QString::fromUtf8(*object.sql)} : std::nullopt});
    }
    const auto &built_schema = manifest->steps.back().expected_schema;
    if (!std::is_permutation(expected_schema.cbegin(), expected_schema.cend(),
                             built_schema.cbegin(), built_schema.cend())) {
        qCritical("Built v22 catalog tuples differ from the sealed projection");
        return 1;
    }

    QCryptographicHash prefix(QCryptographicHash::Sha256);
    for (qsizetype i = 0; i < steps; ++i) {
        const auto &step = manifest->steps[i];
        const auto &expected = seal.steps[i];
        const auto digest = migration_content_sha256(step);
        if (step.version != i + 1 || manifest->supported_starting_versions[i + 1] != i + 1 ||
            step.id.toUtf8().toStdString() != expected.id ||
            digest.toHex().toStdString() != expected.content_sha256_hex) {
            qCritical() << "Compiled catalog differs from the seal at version" << i + 1;
            return 1;
        }
        prefix.addData(digest);
    }
    if (prefix.result().toHex().toStdString() != seal.prefix_sha256_hex) {
        qCritical("Compiled v22 catalog prefix differs from the seal");
        return 1;
    }
    return 0;
}
