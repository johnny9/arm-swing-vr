// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic

TextField {
    id: root
    implicitHeight: Theme.controlHeight
    color: Theme.text
    placeholderTextColor: Theme.tertiary
    selectionColor: Theme.solid
    selectedTextColor: Theme.solidText
    font.pixelSize: Theme.bodySize
    leftPadding: 12
    rightPadding: 12
    selectByMouse: true
    background: Rectangle {
        color: Theme.surface
        radius: Theme.smallRadius
        border.color: root.activeFocus ? Theme.text : Theme.border
        border.width: root.activeFocus ? 2 : 1
    }
}
