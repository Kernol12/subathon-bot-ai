// SPDX-License-Identifier: GPL-3.0-or-later
// Independent Qt-only translation foundation/verifier. No project generator is linked.
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <QTranslator>
#include <array>
#include <iostream>
#include <stdexcept>

namespace {
int assertions = 0;
void require(bool condition, const char *label) {
    if (!condition)
        throw std::runtime_error(label);
    ++assertions;
}
QString literal(const char16_t *value) { return QString::fromUtf16(value); }
void raw(const QTranslator &t, const char *id, int n, const char16_t *expected) {
    require(t.translate("", id, nullptr, n) == literal(expected), "literal raw QM text mismatch");
}
void verify(const QString &directory) {
    QTranslator en, de;
    for (const auto &name : {QStringLiteral("messages_en.qm"), QStringLiteral("messages_de.qm")}) {
        QFileInfo info(QDir(directory).filePath(name));
        require(info.isFile() && !info.isSymLink() && info.size() > 0 && info.size() <= 67108864,
                "QM must be a bounded nonempty regular file");
    }
    require(en.load(QDir(directory).filePath(QStringLiteral("messages_en.qm"))), "English QM load");
    require(de.load(QDir(directory).filePath(QStringLiteral("messages_de.qm"))), "German QM load");
    require(en.language() == QStringLiteral("en") && de.language() == QStringLiteral("de"),
            "actual compiled language identities");
    raw(en, "fixture.greeting", -1, u"Hello, {name}!");
    raw(de, "fixture.greeting", -1, u"Hallo, {name}!");
    require(de.translate("nonempty", "fixture.greeting", nullptr, -1).isNull(), "empty ID context");
    raw(en, "fixture.items", 1, u"{count} item for {name}");
    raw(en, "fixture.items", 2, u"{count} items for {name}");
    raw(de, "fixture.items", 1, u"{name}: {count} Eintrag");
    raw(de, "fixture.items", 2, u"{name}: {count} Einträge");
    raw(de, "fixture.items", 0, u"{name}: {count} Einträge");
    raw(en, "fixture.literal", -1, u"{{x}} %n %1 %L1 %% {name}");
    raw(de, "fixture.literal", -1, u"{{x}} %n %1 %L1 %% {name}");
    raw(en, "fixture.literal_plural", 1, u"{{x}} %n %1 %L1 %% {name}");
    raw(en, "fixture.literal_plural", 2, u"{{x}} %n %1 %L1 %% {name}");
    raw(de, "fixture.literal_plural", 1, u"{{x}} %n %1 %L1 %% {name}");
    raw(de, "fixture.literal_plural", 2, u"{{x}} %n %1 %L1 %% {name}");
    raw(en, "fixture.reorder", -1, u"{first} then {second}; {first}");
    raw(de, "fixture.reorder", -1, u"{second}, danach {first}; {first}");
    raw(en, "fixture.scalars", -1, u"A & B < C > D \"q\" 's'\r\n\tGrüße e\u0301 \U0001F600");
    raw(de, "fixture.scalars", -1, u"A & B < C > D \"q\" 's'\r\n\tGrüße e\u0301 \U0001F600");
    raw(en, "fixture.selector_only", 1, u"One item for {name}");
    raw(en, "fixture.selector_only", 2, u"Items for {name}");
    raw(de, "fixture.selector_only", 1, u"Ein Eintrag für {name}");
    raw(de, "fixture.selector_only", 2, u"Einträge für {name}");
    require(en.translate("", "fixture.absent", nullptr, -1).isNull() &&
                de.translate("", "fixture.absent", nullptr, -1).isNull(),
            "unknown ID remains absent, not invented English");
    QTranslator absent;
    require(!absent.load(QDir(directory).filePath(QStringLiteral("absent_catalog.qm"))),
            "absent selected catalog cannot load");
    auto selected = absent.translate("", "fixture.greeting", nullptr, -1);
    require(selected.isNull(), "absent selected lookup is null");
    if (selected.isNull())
        selected = en.translate("", "fixture.greeting", nullptr, -1);
    require(selected == literal(u"Hello, {name}!"), "explicit actual English base fallback");
    QTranslator missing_base;
    require(!missing_base.load(QDir(directory).filePath(QStringLiteral("absent_english.qm"))),
            "a distinct missing English base cannot load");
    require(missing_base.translate("", "fixture.greeting", nullptr, -1).isNull(),
            "missing English base has no successful text");
}
QByteArray child(const QString &tool, const QStringList &arguments) {
    QProcess process;
    process.start(tool, arguments);
    require(process.waitForStarted(10000), "actual lrelease must start");
    if (!process.waitForFinished(120000)) {
        process.kill();
        process.waitForFinished(10000);
        throw std::runtime_error("actual lrelease timeout");
    }
    const auto out = process.readAllStandardOutput();
    const auto err = process.readAllStandardError();
    require(out.size() + err.size() <= 65536, "bounded foundation tool diagnostics");
    require(process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0,
            "actual lrelease must exit zero");
    return out;
}
QByteArray digest(const QString &path) {
    QFile f(path);
    require(f.open(QIODevice::ReadOnly), "open actual compiled QM");
    QCryptographicHash hash(QCryptographicHash::Sha256);
    while (!f.atEnd()) {
        auto data = f.read(65536);
        require(!data.isEmpty() || f.atEnd(), "bounded QM read succeeds");
        hash.addData(data);
    }
    return hash.result();
}
void same_files(const QString &a, const QString &b) {
    QFile x(a), y(b);
    require(x.open(QIODevice::ReadOnly) && y.open(QIODevice::ReadOnly), "open independent QMs");
    require(x.size() == y.size(), "reproduced QM lengths equal");
    while (!x.atEnd()) {
        const auto left = x.read(65536), right = y.read(65536);
        require(left == right && (!left.isEmpty() || x.atEnd()), "QM bytes reproduce exactly");
    }
    require(y.atEnd(), "complete QM EOF comparison");
}
void foundation(const QString &fixtures, const QString &tool) {
    require(child(tool, {QStringLiteral("-version")}).trimmed() == "lrelease version 6.12.0",
            "selected lrelease exact version");
    QTemporaryDir a, b;
    require(a.isValid() && b.isValid(), "two independent disposable directories");
    const std::array<QString, 2> locales{QStringLiteral("en"), QStringLiteral("de")};
    for (const auto &directory : {a.path(), b.path()}) {
        for (const auto &locale : locales) {
            const auto stem = QStringLiteral("messages_") + locale;
            const auto ts = QDir(directory).filePath(stem + QStringLiteral(".ts"));
            const auto qm = QDir(directory).filePath(stem + QStringLiteral(".qm"));
            require(QFile::copy(QDir(fixtures).filePath(QStringLiteral("goldens/qt/") + stem +
                                                        QStringLiteral(".ts")),
                                ts),
                    "copy independent frozen TS without generator");
            (void)child(tool,
                        {QStringLiteral("-fail-on-unfinished"), ts, QStringLiteral("-qm"), qm});
        }
        verify(directory);
    }
    for (const auto &locale : locales) {
        const auto name = QStringLiteral("messages_") + locale + QStringLiteral(".qm");
        const auto first = QDir(a.path()).filePath(name), second = QDir(b.path()).filePath(name);
        same_files(first, second);
        std::cout << locale.toStdString()
                  << " actual repeated QM SHA256=" << digest(first).toHex().toStdString() << '\n';
    }
}
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        require(QString::fromLatin1(qVersion()) == QStringLiteral("6.12.0"), "selected Qt runtime");
        if (argc == 4 && QByteArray(argv[1]) == "foundation")
            foundation(QString::fromUtf8(argv[2]), QString::fromUtf8(argv[3]));
        else if (argc == 3 && QByteArray(argv[1]) == "verify")
            verify(QString::fromUtf8(argv[2]));
        else
            throw std::runtime_error(
                "usage: translation_catalog_probe foundation FIXTURES LRELEASE | verify QMDIR");
        std::cout << "PASS: " << assertions << " Qt-only literal assertions; no generator linked\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
