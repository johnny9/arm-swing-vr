// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Dialogs
import QtQuick.Layouts

ApplicationWindow {
    id: window
    required property var controller
    width: 1160
    height: 800
    minimumWidth: 900
    minimumHeight: 660
    visible: true
    title: (controller.dirty ? "• " : "") + controller.profile.name + " — Arm Swing VR"
    color: Theme.surface
    font.pixelSize: Theme.bodySize
    property int pageIndex: 0
    property string pendingAction: ""
    property url pendingUrl
    property bool continueAfterSave: false
    property bool allowClose: false
    Binding {
        target: Theme
        property: "dark"
        value: window.controller.darkMode
    }

    function requestAction(action, url) {
        pendingAction = action;
        pendingUrl = url || "";
        if (controller.dirty)
            unsaved.open();
        else
            finishAction();
    }
    function finishAction() {
        const action = pendingAction;
        const url = pendingUrl;
        pendingAction = "";
        if (action === "new")
            controller.newProfile();
        else if (action === "chooseOpen")
            openFile.open();
        else if (action === "open")
            controller.load(url);
        else if (action === "close") {
            allowClose = true;
            close();
        }
    }
    function saveProfile(resume, choosePath) {
        continueAfterSave = resume;
        if (choosePath || controller.fileUrl.toString() === "")
            saveFile.open();
        else if (controller.save(controller.fileUrl) && resume)
            finishAction();
    }
    onClosing: close => {
        if (!allowClose && controller.dirty) {
            close.accepted = false;
            requestAction("close");
        }
    }
    Shortcut {
        sequences: [StandardKey.New]
        onActivated: window.requestAction("new")
    }
    Shortcut {
        sequences: [StandardKey.Open]
        onActivated: window.requestAction("chooseOpen")
    }
    Shortcut {
        sequences: [StandardKey.Save]
        onActivated: window.saveProfile(false, false)
    }
    Shortcut {
        sequences: [StandardKey.SaveAs]
        onActivated: window.saveProfile(false, true)
    }

    RowLayout {
        anchors.fill: parent
        spacing: 0
        Rectangle {
            Layout.preferredWidth: 232
            Layout.fillHeight: true
            color: Theme.sidebar
            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 16
                spacing: 10
                RowLayout {
                    Layout.topMargin: 12
                    Layout.bottomMargin: 22
                    Layout.leftMargin: 8
                    spacing: 10
                    Glyph {
                        name: "motion"
                        width: 25
                        height: 25
                    }
                    Text {
                        text: "Arm Swing VR"
                        color: Theme.text
                        font.pixelSize: 16
                        font.weight: Font.DemiBold
                    }
                }
                AppButton {
                    objectName: "newProfileButton"
                    Layout.fillWidth: true
                    text: "New profile"
                    glyph: "plus"
                    ghost: true
                    onClicked: window.requestAction("new")
                }
                AppButton {
                    Layout.fillWidth: true
                    text: "Open profile"
                    glyph: "folder"
                    ghost: true
                    onClicked: window.requestAction("chooseOpen")
                }
                Text {
                    Layout.topMargin: 24
                    Layout.leftMargin: 12
                    text: "CURRENT PROFILE"
                    font.pixelSize: 10
                    font.letterSpacing: 1
                    color: Theme.tertiary
                }
                AppButton {
                    Layout.fillWidth: true
                    text: window.controller.profile.name || "Untitled profile"
                    glyph: "profile"
                    ghost: true
                    selected: true
                    onClicked: window.pageIndex = 2
                }
                Text {
                    Layout.topMargin: 22
                    Layout.leftMargin: 12
                    text: "RECENT FILES"
                    font.pixelSize: 10
                    font.letterSpacing: 1
                    color: Theme.tertiary
                }
                Text {
                    visible: window.controller.recentProfiles.length === 0
                    Layout.fillWidth: true
                    Layout.margins: 12
                    text: "Profiles you save or open will appear here."
                    font.pixelSize: 12
                    color: Theme.secondary
                    wrapMode: Text.WordWrap
                    lineHeight: 1.3
                }
                ListView {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 4
                    model: window.controller.recentProfiles
                    delegate: AppButton {
                        required property var modelData
                        width: ListView.view.width
                        text: modelData.name
                        glyph: "profile"
                        ghost: true
                        onClicked: window.requestAction("open", modelData.url)
                    }
                }
                Divider {}
                AppButton {
                    objectName: "themeToggle"
                    Layout.fillWidth: true
                    text: Theme.dark ? "Light appearance" : "Dark appearance"
                    glyph: Theme.dark ? "sun" : "moon"
                    ghost: true
                    onClicked: window.controller.darkMode = !window.controller.darkMode
                }
                AppButton {
                    Layout.fillWidth: true
                    text: "About Arm Swing VR"
                    ghost: true
                    onClicked: about.open()
                }
            }
        }
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 0
            RowLayout {
                Layout.fillWidth: true
                Layout.margins: 28
                spacing: 12
                Text {
                    Layout.fillWidth: true
                    text: "Profiles  /  " + (window.controller.profile.name || "Untitled profile")
                    color: Theme.secondary
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
                AppButton {
                    text: "Save as…"
                    ghost: true
                    onClicked: window.saveProfile(false, true)
                }
                AppButton {
                    objectName: "saveButton"
                    text: "Save profile"
                    primary: true
                    onClicked: window.saveProfile(false, false)
                }
            }
            ColumnLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 40
                Layout.rightMargin: 40
                Layout.bottomMargin: 28
                spacing: 12
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        Layout.fillWidth: true
                        text: window.controller.profile.name || "Untitled profile"
                        color: Theme.text
                        font.pixelSize: 30
                        font.weight: Font.DemiBold
                        elide: Text.ElideRight
                    }
                    AppButton {
                        text: "Duplicate"
                        glyph: "copy"
                        ghost: true
                        onClicked: window.controller.duplicateProfile()
                    }
                }
                Text {
                    text: "Movement and controls, tuned for your game."
                    color: Theme.secondary
                    font.pixelSize: 14
                }
                RowLayout {
                    Layout.topMargin: 4
                    spacing: 8
                    Rectangle {
                        implicitWidth: 7
                        implicitHeight: 7
                        radius: 4
                        color: Theme.tertiary
                    }
                    Text {
                        Layout.fillWidth: true
                        text: window.controller.runtimeStatus
                        color: Theme.tertiary
                        font.pixelSize: 12
                        elide: Text.ElideRight
                    }
                    AppButton {
                        objectName: "runtimeToggle"
                        text: window.controller.runtimeArmed ? "Stop input" : "Enable profile"
                        primary: window.controller.runtimeArmed
                        onClicked: window.controller.runtimeArmed ? window.controller.stopRuntime() : window.controller.startRuntime()
                    }
                    AppButton {
                        text: "Launch setup"
                        ghost: true
                        onClicked: launchSetup.open()
                    }
                }
            }
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 32
                Layout.rightMargin: 32
                Layout.bottomMargin: 12
                spacing: 4
                AppButton {
                    objectName: "movementTab"
                    text: "Arm swing"
                    glyph: "motion"
                    ghost: true
                    selected: window.pageIndex === 0
                    onClicked: window.pageIndex = 0
                }
                AppButton {
                    objectName: "mappingsTab"
                    text: "Controller mappings"
                    glyph: "mapping"
                    ghost: true
                    selected: window.pageIndex === 1
                    onClicked: window.pageIndex = 1
                }
                AppButton {
                    objectName: "gameTab"
                    text: "Game setup"
                    ghost: true
                    selected: window.pageIndex === 2
                    onClicked: window.pageIndex = 2
                }
                Item {
                    Layout.fillWidth: true
                }
            }
            Divider {}
            ScrollView {
                id: scroll
                objectName: "editorScroll"
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                contentWidth: availableWidth
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                Column {
                    width: scroll.availableWidth
                    spacing: 0
                    Item {
                        width: 1
                        height: 30
                    }
                    Loader {
                        id: pageLoader
                        x: 40
                        width: parent.width - 80
                        sourceComponent: window.pageIndex === 0 ? movementPage : window.pageIndex === 1 ? mappingsPage : gamePage
                    }
                    Item {
                        width: 1
                        height: 36
                    }
                }
            }
            Divider {}
            RowLayout {
                Layout.fillWidth: true
                Layout.leftMargin: 32
                Layout.rightMargin: 32
                Layout.topMargin: 16
                Layout.bottomMargin: 16
                spacing: 8
                Glyph {
                    name: window.controller.dirty ? "profile" : "check"
                    stroke: Theme.secondary
                    width: 16
                    height: 16
                }
                Text {
                    objectName: "saveStatus"
                    Layout.fillWidth: true
                    text: window.controller.dirty ? "Unsaved changes" : window.controller.status
                    color: Theme.secondary
                    font.pixelSize: 12
                    elide: Text.ElideRight
                }
                Text {
                    text: "VR INPUT PROTOTYPE"
                    color: Theme.tertiary
                    font.pixelSize: 10
                    font.letterSpacing: 0.6
                }
            }
        }
    }
    Component {
        id: movementPage
        MovementPage {
            controller: window.controller
        }
    }
    Component {
        id: mappingsPage
        MappingsPage {
            controller: window.controller
        }
    }
    Component {
        id: gamePage
        GamePage {
            controller: window.controller
        }
    }
    FileDialog {
        id: openFile
        title: "Open profile"
        nameFilters: ["Profiles (*.json)"]
        onAccepted: window.controller.load(selectedFile)
    }
    FileDialog {
        id: saveFile
        title: "Save profile"
        fileMode: FileDialog.SaveFile
        defaultSuffix: "json"
        nameFilters: ["Profiles (*.json)"]
        onAccepted: {
            if (window.controller.save(selectedFile) && window.continueAfterSave)
                window.finishAction();
        }
        onRejected: {
            window.continueAfterSave = false;
            window.pendingAction = "";
        }
    }
    Dialog {
        id: unsaved
        objectName: "unsavedDialog"
        anchors.centerIn: parent
        width: 420
        modal: true
        padding: 24
        background: Rectangle {
            color: Theme.elevated
            radius: 16
            border.color: Theme.border
        }
        contentItem: ColumnLayout {
            spacing: 20
            Text {
                text: "Save your changes?"
                color: Theme.text
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }
            Text {
                Layout.fillWidth: true
                text: "This profile has changes that haven’t been saved."
                color: Theme.secondary
                font.pixelSize: 14
                wrapMode: Text.WordWrap
            }
            RowLayout {
                Layout.fillWidth: true
                AppButton {
                    objectName: "cancelDiscard"
                    text: "Cancel"
                    ghost: true
                    onClicked: {
                        unsaved.close();
                        window.pendingAction = "";
                    }
                }
                Item {
                    Layout.fillWidth: true
                }
                AppButton {
                    objectName: "discardChanges"
                    text: "Discard"
                    onClicked: {
                        unsaved.close();
                        window.finishAction();
                    }
                }
                AppButton {
                    text: "Save"
                    primary: true
                    onClicked: {
                        unsaved.close();
                        window.saveProfile(true, false);
                    }
                }
            }
        }
    }
    Dialog {
        id: errorDialog
        anchors.centerIn: parent
        width: 420
        modal: true
        padding: 24
        property string message: ""
        background: Rectangle {
            color: Theme.elevated
            radius: 16
            border.color: Theme.border
        }
        contentItem: ColumnLayout {
            spacing: 20
            Text {
                text: "Check this profile"
                color: Theme.text
                font.pixelSize: 20
                font.weight: Font.DemiBold
            }
            Text {
                Layout.fillWidth: true
                text: errorDialog.message
                color: Theme.secondary
                font.pixelSize: 14
                wrapMode: Text.WordWrap
            }
            AppButton {
                text: "Got it"
                primary: true
                Layout.alignment: Qt.AlignRight
                onClicked: errorDialog.close()
            }
        }
    }
    Connections {
        target: window.controller
        function onErrorOccurred(message) {
            errorDialog.message = message;
            errorDialog.open();
        }
    }
    Dialog {
        id: launchSetup
        anchors.centerIn: parent
        width: Math.min(680, window.width - 48)
        modal: true
        padding: 24
        background: Rectangle {
            color: Theme.elevated
            radius: 16
            border.color: Theme.border
        }
        contentItem: ColumnLayout {
            spacing: 20
            Text {
                text: "Connect this game"
                color: Theme.text
                font.pixelSize: 22
                font.weight: Font.DemiBold
            }
            Text {
                Layout.fillWidth: true
                text: "Native Linux launch options are shown below. OpenVR games that load their API directly also need the reversible per-game installer in docs/VR-INTEGRATION.md. Set the game to head-relative smooth locomotion and keep this editor open; editing the profile stops input."
                color: Theme.secondary
                wrapMode: Text.WordWrap
            }
            TextArea {
                Layout.fillWidth: true
                Layout.preferredHeight: 150
                text: window.controller.launchCommand
                readOnly: true
                selectByMouse: true
                wrapMode: TextEdit.WrapAnywhere
                color: Theme.text
                font.pixelSize: 12
                background: Rectangle {
                    radius: 8
                    color: Theme.sidebar
                    border.color: Theme.border
                }
            }
            Text {
                Layout.fillWidth: true
                text: "Proton games need the Windows backend and separate setup. See docs/VR-INTEGRATION.md. This prototype supports Index A/B clicks and simple joystick bindings; game compatibility still needs headset testing."
                color: Theme.secondary
                wrapMode: Text.WordWrap
            }
            AppButton {
                Layout.alignment: Qt.AlignRight
                text: "Done"
                primary: true
                onClicked: launchSetup.close()
            }
        }
    }
    Dialog {
        id: about
        anchors.centerIn: parent
        width: 460
        modal: true
        padding: 24
        background: Rectangle {
            color: Theme.elevated
            radius: 16
            border.color: Theme.border
        }
        contentItem: ColumnLayout {
            spacing: 20
            Text {
                text: "Arm Swing VR"
                color: Theme.text
                font.pixelSize: 24
                font.weight: Font.DemiBold
            }
            Text {
                Layout.fillWidth: true
                text: "Free software under the GNU GPL version 3 or later. You may modify and redistribute it under that license. This program comes with no warranty.\n\nCopyright © 2026 Arm Swing VR contributors.\n\nSelected design tokens adapted from OpenAI’s MIT-licensed Apps SDK UI. Native QML components and icons by Arm Swing VR contributors. No OpenAI affiliation.\n\nBuilt with Qt 6 under its open-source licenses. See the distributed license notices."
                color: Theme.secondary
                font.pixelSize: 13
                wrapMode: Text.WordWrap
            }
            RowLayout {
                AppButton {
                    text: "Source & licenses"
                    onClicked: Qt.openUrlExternally("https://github.com/johnny9/arm-swing-vr")
                }
                Item {
                    Layout.fillWidth: true
                }
                AppButton {
                    text: "Done"
                    primary: true
                    onClicked: about.close()
                }
            }
        }
    }
}
