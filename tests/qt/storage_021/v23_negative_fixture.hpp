// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once

#include "sqlite_v23_rebuild.hpp"

#include <QStringList>

namespace subathon::server::detail::testing {

struct V23NegativeSpecimen final {
    enum class Phase { before_history, after_history };

    V23NegativeSpecimen(QString path, Phase phase);
    ~V23NegativeSpecimen();
    V23NegativeSpecimen(const V23NegativeSpecimen &) = delete;
    V23NegativeSpecimen &operator=(const V23NegativeSpecimen &) = delete;

    const QString path;
    const Phase phase;
    QStringList attempts;
    qsizetype injection_after{};
    bool orphan_verified{};
    bool foreign_keys_restored{};
};

// Wraps the fenced real writer port only in the negative executable.
class V23NegativeOwner final {
  public:
    explicit V23NegativeOwner(V23OwnerConnection &owner);
    ReadResult read(const QString &sql);
    V23OwnerConnection::Control transaction(V23OwnerConnection::Transaction operation);
    QString path() const { return owner_.path(); }
    void before_history();

  private:
    void inject_orphan();

    V23OwnerConnection &owner_;
    V23NegativeSpecimen *const specimen_;
};

} // namespace subathon::server::detail::testing
