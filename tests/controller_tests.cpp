// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "profile_controller.h"
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QtTest>

class ControllerTests : public QObject {
    Q_OBJECT
  private slots:
    void editingMappingsDoesNotResetTheModel() {
        MappingModel model;
        model.add();
        QSignalSpy reset(&model, &MappingModel::modelReset);
        QSignalSpy changed(&model, &MappingModel::dataChanged);
        model.setInput(0, true, "left-a");
        model.setInput(0, false, "right-b");
        QCOMPARE(reset.count(), 0);
        QCOMPARE(changed.count(), 2);
        QCOMPARE(model.entries()[0].source, QString("left-a"));
    }
    void saveLoadAndDuplicatePreserveIndependentSettings() {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        ProfileController controller(dir.filePath("ui.ini"));
        QVERIFY(!controller.dirty());
        controller.setField("name", "Game one");
        controller.setField("output_hand", "right");
        controller.mappings()->add();
        controller.mappings()->setInput(0, true, "left-a");
        controller.mappings()->setInput(0, false, "right-b");
        QVERIFY(controller.dirty());
        const auto file = QUrl::fromLocalFile(dir.filePath("one.json"));
        QVERIFY(controller.save(file));
        QVERIFY(!controller.dirty());
        QCOMPARE(controller.recentProfiles().size(), 1);
        controller.newProfile();
        QCOMPARE(controller.mappings()->rowCount(), 0);
        QVERIFY(controller.load(file));
        QCOMPARE(controller.profile()["output_hand"].toString(), QString("right"));
        QCOMPARE(controller.profile()["contributing_arms"].toString(), QString("both"));
        QCOMPARE(controller.mappings()->entries()[0].destination, QString("right-b"));
        controller.duplicateProfile();
        QVERIFY(controller.dirty());
        QVERIFY(controller.fileUrl().isEmpty());
        QCOMPARE(controller.profile()["name"].toString(), QString("Game one (copy)"));
        QVERIFY(controller.load(file));
        QCOMPARE(controller.profile()["name"].toString(), QString("Game one"));
    }
    void failedSaveKeepsDirtyStateAndDestination() {
        QTemporaryDir dir;
        ProfileController controller(dir.filePath("ui.ini"));
        const auto file = QUrl::fromLocalFile(dir.filePath("one.json"));
        QVERIFY(controller.save(file));
        controller.setField("name", "");
        QSignalSpy errors(&controller, &ProfileController::errorOccurred);
        QVERIFY(!controller.save(QUrl::fromLocalFile(dir.filePath("two.json"))));
        QCOMPARE(errors.count(), 1);
        QCOMPARE(controller.fileUrl(), file);
        QVERIFY(controller.dirty());
        QVERIFY(!QFileInfo::exists(dir.filePath("two.json")));
    }
    void themeAndRecentFilesSurviveRestart() {
        QTemporaryDir dir;
        const auto settings = dir.filePath("ui.ini");
        {
            ProfileController controller(settings);
            controller.setDarkMode(false);
            QVERIFY(controller.save(QUrl::fromLocalFile(dir.filePath("profile.json"))));
        }
        ProfileController restored(settings);
        QVERIFY(!restored.darkMode());
        QCOMPARE(restored.recentProfiles().size(), 1);
    }
};
QTEST_GUILESS_MAIN(ControllerTests)
#include "controller_tests.moc"
