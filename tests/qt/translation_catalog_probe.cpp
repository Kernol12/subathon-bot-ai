// SPDX-License-Identifier: GPL-3.0-or-later
// Checks the actual generated QM with literal expected text; no generator is linked.
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QTranslator>
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
} // namespace
int main(int argc, char **argv) {
    QCoreApplication app(argc, argv);
    try {
        require(QString::fromLatin1(qVersion()) == QStringLiteral("6.12.0"), "selected Qt runtime");
        if (argc == 3 && QByteArray(argv[1]) == "verify")
            verify(QString::fromUtf8(argv[2]));
        else
            throw std::runtime_error(
                "usage: translation_catalog_probe verify QMDIR");
        std::cout << "PASS: " << assertions << " Qt-only literal assertions; no generator linked\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << "FAIL: " << e.what() << '\n';
        return 1;
    }
}
