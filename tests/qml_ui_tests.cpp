// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
#include "profile_controller.h"
#include <QDir>
#include <QQmlApplicationEngine>
#include <QQuickItem>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QWheelEvent>
#include <QtTest>
#include <memory>

class QmlUiTests : public QObject {
    Q_OBJECT
  private:
    QTemporaryDir directory_;
    std::unique_ptr<ProfileController> controller_;
    std::unique_ptr<QQmlApplicationEngine> engine_;
    QQuickWindow* window_ = nullptr;
    QQuickItem* findVisual(QQuickItem* parent, const char* name) {
        if (parent->objectName() == QString::fromLatin1(name))
            return parent;
        for (auto* child : parent->childItems()) {
            if (auto* found = findVisual(child, name))
                return found;
        }
        return nullptr;
    }
    QQuickItem* item(const char* name) { return findVisual(window_->contentItem(), name); }
    void click(const char* name) {
        auto* target = item(name);
        QVERIFY2(target, name);
        QVERIFY2(target->isVisible(), name);
        QTest::mouseClick(
            window_, Qt::LeftButton, {},
            target->mapToScene(QPointF(target->width() / 2, target->height() / 2)).toPoint());
    }
  private slots:
    void initTestCase() {
        QQuickStyle::setStyle("Basic");
        QVERIFY(directory_.isValid());
    }
    void init() {
        controller_ = std::make_unique<ProfileController>(
            directory_.filePath(QString::fromLatin1(QTest::currentTestFunction()) + ".ini"));
        engine_ = std::make_unique<QQmlApplicationEngine>();
        engine_->setInitialProperties({{"controller", QVariant::fromValue(controller_.get())}});
        engine_->load(QUrl("qrc:/qml/Main.qml"));
        QVERIFY(!engine_->rootObjects().isEmpty());
        window_ = qobject_cast<QQuickWindow*>(engine_->rootObjects().first());
        QVERIFY(window_);
        window_->requestActivate();
        QVERIFY(QTest::qWaitForWindowExposed(window_));
    }
    void cleanup() {
        window_ = nullptr;
        engine_.reset();
        controller_.reset();
    }
    void keyboardEditsReachTheSavedProfile() {
        click("gameTab");
        QTRY_VERIFY(item("profileNameField"));
        auto* field = item("profileNameField");
        field->forceActiveFocus();
        QTest::keyClick(window_, Qt::Key_A, Qt::ControlModifier);
        for (const auto character : QString("My game"))
            QTest::keyClick(window_, character.toLatin1());
        QCOMPARE(controller_->profile()["name"].toString(), QString("My game"));
        const auto file = QUrl::fromLocalFile(directory_.filePath("keyboard.json"));
        QVERIFY(controller_->save(file));
        click("movementTab");
        QTRY_VERIFY(item("outputCombo"));
        item("outputCombo")->forceActiveFocus();
        QTRY_VERIFY(item("outputCombo")->hasActiveFocus());
        QCOMPARE(item("outputCombo")->property("currentIndex").toInt(), 0);
        QTest::keyClick(window_, Qt::Key_Down);
        QCOMPARE(controller_->profile()["output_hand"].toString(), QString("right"));
        QCOMPARE(controller_->profile()["contributing_arms"].toString(), QString("both"));
        QTest::keyClick(window_, Qt::Key_S, Qt::ControlModifier);
        QTRY_VERIFY(!controller_->dirty());
        armswing::Profile restored;
        QString error;
        QVERIFY(armswing::loadProfile(file.toLocalFile(), restored, error));
        QCOMPARE(restored.outputHand, QString("right"));
    }
    void mappingsCanBeAddedAndRemovedFromQml() {
        click("mappingsTab");
        QTRY_VERIFY(item("addMapping"));
        click("addMapping");
        QCOMPARE(controller_->mappings()->rowCount(), 1);
        QTRY_VERIFY(item("removeMapping"));
        click("removeMapping");
        QCOMPARE(controller_->mappings()->rowCount(), 0);
    }
    void popupSelectionUpdatesTheDraft() {
        click("armsCombo");
        QTRY_VERIFY(item("comboOption_1") && item("comboOption_1")->isVisible());
        click("comboOption_1");
        QCOMPARE(controller_->profile()["contributing_arms"].toString(), QString("left"));
        QCOMPARE(controller_->profile()["output_hand"].toString(), QString("left"));
    }
    void compactWindowCanScrollToLowerSettings() {
        window_->resize(900, 660);
        window_->setProperty("pageIndex", 2);
        auto* scroll = item("editorScroll");
        QVERIFY(scroll);
        auto* flickable = scroll->property("contentItem").value<QObject*>();
        QVERIFY(flickable);
        QTRY_VERIFY(flickable->property("contentHeight").toDouble() > scroll->height());
        QTest::qWait(100);
        const QPointF position(360, 480);
        QTest::mouseMove(window_, position.toPoint());
        QWheelEvent wheel(position, window_->mapToGlobal(position.toPoint()), {}, QPoint(0, -240),
                          Qt::NoButton, Qt::NoModifier, Qt::NoScrollPhase, false);
        QCoreApplication::sendEvent(window_, &wheel);
        QTRY_VERIFY(flickable->property("contentY").toDouble() > 0);
    }
    void customInputCanBeClearedAndRetyped() {
        click("inputPickerCombo");
        QTRY_VERIFY(item("comboOption_5") && item("comboOption_5")->isVisible());
        click("comboOption_5");
        auto* field = item("customInputPath");
        QVERIFY(field);
        QTRY_VERIFY(field->isVisible());
        field->forceActiveFocus();
        QTest::keyClick(window_, Qt::Key_A, Qt::ControlModifier);
        QTest::keyClick(window_, Qt::Key_Backspace);
        QVERIFY(field->isVisible());
        QVERIFY(field->hasActiveFocus());
        QCOMPARE(controller_->profile()["activation_input"].toString(), QString());
        for (const auto character : QString("custom-input"))
            QTest::keyClick(window_, character.toLatin1());
        QCOMPARE(controller_->profile()["activation_input"].toString(), QString("custom-input"));
        controller_->newProfile();
        QTRY_VERIFY(!field->isVisible());
    }
    void unsavedChangesCanBeCancelledOrDiscarded() {
        controller_->setField("name", "Keep my work");
        click("newProfileButton");
        QTRY_VERIFY(item("cancelDiscard") && item("cancelDiscard")->isVisible());
        click("cancelDiscard");
        QCOMPARE(controller_->profile()["name"].toString(), QString("Keep my work"));
        QVERIFY(controller_->dirty());
        click("newProfileButton");
        QTRY_VERIFY(item("discardChanges") && item("discardChanges")->isVisible());
        click("discardChanges");
        QCOMPARE(controller_->profile()["name"].toString(), QString("New profile"));
        QVERIFY(!controller_->dirty());
    }
    void renderLayouts_data() {
        QTest::addColumn<bool>("dark");
        QTest::addColumn<QSize>("size");
        QTest::addColumn<int>("page");
        QTest::newRow("dark-movement") << true << QSize(1160, 860) << 0;
        QTest::newRow("light-movement") << false << QSize(1160, 860) << 0;
        QTest::newRow("compact-game") << true << QSize(900, 660) << 2;
        QTest::newRow("dark-mappings") << true << QSize(1160, 860) << 1;
    }
    void renderLayouts() {
        QFETCH(bool, dark);
        QFETCH(QSize, size);
        QFETCH(int, page);
        controller_->setDarkMode(dark);
        controller_->setField("name", "Half-Life 2 VR");
        controller_->setField("steam_app_id", "658920");
        if (page == 1) {
            controller_->mappings()->add();
            controller_->mappings()->setInput(0, true, "/user/hand/left/input/a/click");
            controller_->mappings()->setInput(0, false, "/user/hand/right/input/b/click");
        }
        window_->resize(size);
        window_->setProperty("pageIndex", page);
        QTest::qWait(150);
        const auto image = window_->grabWindow();
        QVERIFY(!image.isNull());
        auto* save = item("saveButton");
        QVERIFY(save);
        auto* label = findVisual(save, "buttonLabel");
        QVERIFY(label);
        QCOMPARE(label->property("color").value<QColor>(), QColor(dark ? "#0d0d0d" : "#ffffff"));
        const auto position = save->mapToScene(QPointF(0, 0));
        QVERIFY(position.x() >= 0 && position.x() + save->width() <= window_->width());
        QVERIFY(position.y() >= 0 && position.y() + save->height() <= window_->height());
        const auto output = qEnvironmentVariable("ARMSWING_TEST_SCREENSHOTS");
        if (!output.isEmpty()) {
            QVERIFY(QDir().mkpath(output));
            QVERIFY(image.save(
                QDir(output).filePath(QString::fromLatin1(QTest::currentDataTag()) + ".png")));
        }
    }
};
QTEST_MAIN(QmlUiTests)
#include "qml_ui_tests.moc"
