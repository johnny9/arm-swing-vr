// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: page
    required property var controller
    spacing: 24
    RowLayout {
        Layout.fillWidth: true
        Text {
            Layout.fillWidth: true
            text: "Controller mappings"
            color: Theme.text
            font.pixelSize: 22
            font.weight: Font.DemiBold
        }
        AppButton {
            objectName: "addMapping"
            text: "Add mapping"
            glyph: "plus"
            onClicked: page.controller.mappings.add()
        }
    }
    Text {
        Layout.fillWidth: true
        text: "Keep a game action available on another controller input."
        color: Theme.secondary
        font.pixelSize: 14
        wrapMode: Text.WordWrap
        Layout.topMargin: -12
    }
    Divider {}
    ColumnLayout {
        visible: rows.count === 0
        Layout.fillWidth: true
        Layout.topMargin: 52
        Layout.bottomMargin: 64
        spacing: 16
        Rectangle {
            Layout.alignment: Qt.AlignHCenter
            implicitWidth: 64
            implicitHeight: 64
            radius: 20
            color: Theme.elevated
            Glyph {
                anchors.centerIn: parent
                width: 28
                height: 28
                name: "mapping"
                stroke: Theme.secondary
            }
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            text: "No mappings yet"
            color: Theme.text
            font.pixelSize: 18
            font.weight: Font.DemiBold
        }
        Text {
            Layout.alignment: Qt.AlignHCenter
            Layout.fillWidth: true
            text: "Add the input you want to change, then choose where it should go."
            color: Theme.secondary
            font.pixelSize: 13
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }
    }
    Repeater {
        id: rows
        model: page.controller.mappings
        delegate: ColumnLayout {
            id: entry
            required property int index
            required property string sourceInput
            required property string destinationInput
            Layout.fillWidth: true
            spacing: 14
            RowLayout {
                Layout.fillWidth: true
                spacing: 12
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        text: "From"
                        color: Theme.secondary
                        font.pixelSize: 12
                    }
                    InputPicker {
                        Layout.fillWidth: true
                        currentPath: entry.sourceInput
                        accessibleLabel: "Mapping source"
                        onPathSelected: path => page.controller.mappings.setInput(entry.index, true, path)
                    }
                }
                Glyph {
                    name: "arrow"
                    stroke: Theme.tertiary
                    Layout.topMargin: 24
                }
                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8
                    Text {
                        text: "To"
                        color: Theme.secondary
                        font.pixelSize: 12
                    }
                    InputPicker {
                        Layout.fillWidth: true
                        currentPath: entry.destinationInput
                        accessibleLabel: "Mapping destination"
                        onPathSelected: path => page.controller.mappings.setInput(entry.index, false, path)
                    }
                }
                AppButton {
                    objectName: "removeMapping"
                    text: ""
                    glyph: "close"
                    ghost: true
                    Accessible.name: "Remove mapping"
                    Layout.topMargin: 24
                    onClicked: page.controller.mappings.remove(entry.index)
                }
            }
            Divider {}
        }
    }
    Text {
        Layout.fillWidth: true
        text: "Live mappings currently support Index A/B clicks. Map another physical input to the activation button's original game action to keep that action available."
        color: Theme.tertiary
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }
}
