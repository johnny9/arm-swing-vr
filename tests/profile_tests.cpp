// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include <armswing/profile.h>

#include <QFile>
#include <QJsonArray>
#include <QTemporaryDir>
#include <QtTest>
#include <limits>

class ProfileTests : public QObject {
    Q_OBJECT
  private slots:
    void roundTripKeepsGameAndMappingsTogether() {
        armswing::Profile original;
        original.name = "Example game";
        original.steamAppId = "1234";
        original.runtime = "openxr";
        original.contributingArms = "both";
        original.outputHand = "right";
        original.steering = "left-hand";
        original.sensitivity = 1.75;
        original.mappings.append(armswing::ButtonMapping{"/user/hand/left/input/a/click",
                                                         "/user/hand/right/input/b/click"});
        armswing::Profile restored;
        QString error;
        QVERIFY2(armswing::fromJson(armswing::toJson(original), restored, error),
                 qPrintable(error));
        QVERIFY(restored == original);
    }

    void badDocumentsLeaveCurrentProfileUntouched_data() {
        QTest::addColumn<QJsonObject>("document");
        const auto valid = armswing::toJson({});
        auto invalid = valid;
        invalid["schema_version"] = 2;
        QTest::newRow("future-version") << invalid;
        invalid = valid;
        invalid["sensitivity"] = "1.0";
        QTest::newRow("wrong-type") << invalid;
        invalid = valid;
        invalid["sensitivity"] = 1.234;
        QTest::newRow("unsupported-precision") << invalid;
        invalid = valid;
        invalid["runtime"] = "unknown";
        QTest::newRow("unknown-runtime") << invalid;
        invalid = valid;
        invalid["future_setting"] = true;
        QTest::newRow("unknown-field") << invalid;
        invalid = valid;
        invalid.remove("activation_input");
        QTest::newRow("missing-activation") << invalid;
        invalid = valid;
        invalid["steam_app_id"] = "1234abc";
        QTest::newRow("invalid-app-id") << invalid;
        invalid = valid;
        invalid["mappings"] = QJsonArray{QJsonObject{{"source", "a"}, {"destination", "b"}},
                                         QJsonObject{{"source", "a"}, {"destination", "c"}}};
        QTest::newRow("conflicting-mappings") << invalid;
    }

    void badDocumentsLeaveCurrentProfileUntouched() {
        QFETCH(QJsonObject, document);
        armswing::Profile current;
        current.name = "Unsaved work";
        const auto before = current;
        QString error;
        QVERIFY(!armswing::fromJson(document, current, error));
        QVERIFY(!error.isEmpty());
        QVERIFY(current == before);
    }

    void invalidSaveDoesNotOverwriteExistingProfile() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        const auto path = directory.filePath("game.json");
        armswing::Profile original;
        original.name = "Preserve me";
        QString error;
        QVERIFY2(armswing::saveProfile(path, original, error), qPrintable(error));
        auto invalid = original;
        invalid.sensitivity = std::numeric_limits<double>::quiet_NaN();
        QVERIFY(!armswing::saveProfile(path, invalid, error));
        armswing::Profile restored;
        QVERIFY2(armswing::loadProfile(path, restored, error), qPrintable(error));
        QVERIFY(restored == original);
        QVERIFY(error.isEmpty());
    }

    void filesKeepDifferentGamesIndependent() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        armswing::Profile first;
        first.name = "First game";
        first.steamAppId = "100";
        armswing::Profile second = first;
        second.name = "Second game";
        second.steamAppId = "200";
        second.outputHand = "right";
        second.mappings.append(armswing::ButtonMapping{"a", "b"});
        QString error;
        QVERIFY(armswing::saveProfile(directory.filePath("first.json"), first, error));
        QVERIFY(armswing::saveProfile(directory.filePath("second.json"), second, error));
        armswing::Profile restored;
        QVERIFY(armswing::loadProfile(directory.filePath("first.json"), restored, error));
        QVERIFY(restored == first);
        QVERIFY(armswing::loadProfile(directory.filePath("second.json"), restored, error));
        QVERIFY(restored == second);
    }

    void malformedFileDoesNotReplaceCurrentProfile() {
        QTemporaryDir directory;
        QVERIFY(directory.isValid());
        QFile file(directory.filePath("broken.json"));
        QVERIFY(file.open(QIODevice::WriteOnly));
        file.write("{ broken");
        file.close();
        armswing::Profile current;
        current.name = "Keep me";
        const auto before = current;
        QString error;
        QVERIFY(!armswing::loadProfile(file.fileName(), current, error));
        QVERIFY(!error.isEmpty());
        QVERIFY(current == before);
    }
};

QTEST_GUILESS_MAIN(ProfileTests)
#include "profile_tests.moc"
