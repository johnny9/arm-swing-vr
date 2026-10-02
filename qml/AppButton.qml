// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

Button {
    id: root
    property bool primary: false
    property bool ghost: false
    property bool selected: false
    property string glyph: ""
    implicitHeight: Theme.controlHeight
    implicitWidth: contentItem.implicitWidth + 28
    leftPadding: 14
    rightPadding: 14
    hoverEnabled: true
    font.pixelSize: Theme.bodySize
    Accessible.name: text
    background: Rectangle {
        radius: Theme.smallRadius
        color: root.primary ? (root.hovered ? Theme.secondary : Theme.solid) : root.hovered || root.selected ? Theme.hover : root.ghost ? "transparent" : Theme.elevated
        border.width: root.activeFocus ? 2 : root.primary || root.ghost ? 0 : 1
        border.color: root.activeFocus ? Theme.text : Theme.border
        opacity: root.enabled ? 1 : 0.4
        Behavior on color {
            ColorAnimation {
                duration: 100
            }
        }
    }
    contentItem: RowLayout {
        spacing: 8
        Glyph {
            visible: root.glyph !== ""
            name: root.glyph
            stroke: root.primary ? Theme.solidText : Theme.text
            Layout.preferredWidth: 18
            Layout.preferredHeight: 18
        }
        Text {
            objectName: "buttonLabel"
            text: root.text
            color: root.primary ? Theme.solidText : Theme.text
            font: root.font
            elide: Text.ElideRight
            Layout.fillWidth: true
            horizontalAlignment: Text.AlignHCenter
            opacity: root.enabled ? 1 : 0.5
        }
    }
}
