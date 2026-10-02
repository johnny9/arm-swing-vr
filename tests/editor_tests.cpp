// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "profile_editor.h"

#include <QComboBox>
#include <QTableWidget>
#include <QtTest>

class EditorTests : public QObject {
    Q_OBJECT
  private slots:
    void switchingProfilesReplacesMappingsAndSettings() {
        ProfileEditor editor;
        armswing::Profile first;
        first.name = "First game";
        first.runtime = "openxr";
        first.outputHand = "right";
        first.contributingArms = "left";
        first.mappings.append(armswing::ButtonMapping{"a", "b"});
        editor.setProfile(first);
        QVERIFY(editor.profile() == first);
        armswing::Profile second;
        second.name = "Second game";
        editor.setProfile(second);
        QVERIFY(editor.profile() == second);
        QCOMPARE(editor.findChild<QTableWidget*>()->rowCount(), 0);
    }

    void changingOutputDoesNotChangeContributingArms() {
        ProfileEditor editor;
        editor.show();
        QComboBox* output = nullptr;
        for (auto* combo : editor.findChildren<QComboBox*>()) {
            if (combo->count() == 2 && combo->itemData(0) == "left")
                output = combo;
        }
        QVERIFY(output);
        output->setFocus();
        QTest::keyClick(output, Qt::Key_Down);
        QCOMPARE(editor.profile().outputHand, QString("right"));
        QCOMPARE(editor.profile().contributingArms, QString("both"));
    }
};

QTEST_MAIN(EditorTests)
#include "editor_tests.moc"
