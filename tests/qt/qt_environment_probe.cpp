// SPDX-License-Identifier: GPL-3.0-or-later
#include <QObject>
#include <QTest>
#include <QTimer>
class QtEnvironmentProbe final : public QObject {
    Q_OBJECT
private slots:
    void eventLoopRunsQueuedWork() {
        bool fired = false;
        QTimer::singleShot(0, this, [&fired] { fired = true; });
        QTRY_VERIFY(fired);
    }
};
QTEST_GUILESS_MAIN(QtEnvironmentProbe)
#include "qt_environment_probe.moc"
