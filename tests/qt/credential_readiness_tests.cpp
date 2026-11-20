// SPDX-License-Identifier: GPL-3.0-or-later
#include "../support/credential_native_controls.hpp"
#include "support/credential_key_fixture.hpp"

using namespace credential_test_support;
using subathon::server::CredentialError;
namespace native = credential_native;

class CredentialReadinessTests final : public QObject {
    Q_OBJECT

  private slots:
    // A failed readiness result may remain cached for this process.
    void readinessFailure_SC09() {
        checked([&] {
            KeyFixture keys;
            const auto path = keys.file();
            KeyFixture::write(path);
            const native::Controls controls(native::Fault::readiness);
            expect_error(provision_path(path), CredentialError::crypto_unavailable);
            QVERIFY(controls.observation().readiness > 0);
        });
    }
};

QTEST_GUILESS_MAIN(CredentialReadinessTests)
#include "credential_readiness_tests.moc"
