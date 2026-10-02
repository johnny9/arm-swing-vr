// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

RowLayout {
    id: root
    property string title: ""
    property string description: ""
    default property alias controls: controlBox.data
    spacing: 24
    Layout.fillWidth: true
    data: [
        ColumnLayout {
            Layout.fillWidth: true
            Layout.minimumWidth: 150
            spacing: 5
            Text {
                Layout.fillWidth: true
                text: root.title
                color: Theme.text
                font.pixelSize: Theme.bodySize
                wrapMode: Text.WordWrap
            }
            Text {
                Layout.fillWidth: true
                visible: text.length > 0
                text: root.description
                color: Theme.secondary
                font.pixelSize: Theme.smallSize
                wrapMode: Text.WordWrap
                lineHeight: 1.25
            }
        },
        ColumnLayout {
            id: controlBox
            Layout.preferredWidth: 240
            Layout.maximumWidth: 280
            Layout.fillWidth: true
            spacing: 8
        }
    ]
}
