// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic

ComboBox {
    id: root
    property var selectedValue
    currentIndex: {
        // Wait for the control model before resolving a stored value.
        if (count === 0 || !model)
            return -1;
        for (let i = 0; i < model.length; ++i)
            if (model[i].value === selectedValue)
                return i;
        return -1;
    }
    implicitHeight: Theme.controlHeight
    textRole: "label"
    valueRole: "value"
    font.pixelSize: Theme.bodySize
    leftPadding: 12
    rightPadding: 36
    Accessible.name: displayText
    contentItem: Text {
        text: root.displayText
        color: Theme.text
        font: root.font
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }
    indicator: Glyph {
        name: "chevron"
        stroke: Theme.secondary
        x: root.width - width - 10
        y: (root.height - height) / 2
        width: 16
        height: 16
    }
    background: Rectangle {
        radius: Theme.smallRadius
        color: root.hovered ? Theme.elevated : Theme.surface
        border.color: root.activeFocus ? Theme.text : Theme.border
        border.width: root.activeFocus ? 2 : 1
    }
    delegate: ItemDelegate {
        objectName: "comboOption_" + index
        width: root.width
        implicitHeight: 40
        text: modelData.label
        font.pixelSize: Theme.bodySize
        contentItem: Text {
            text: modelData.label
            color: Theme.text
            font: parent.font
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }
        background: Rectangle {
            radius: 6
            color: parent.highlighted || parent.hovered ? Theme.hover : "transparent"
        }
        highlighted: root.highlightedIndex === index
    }
    popup: Popup {
        y: root.height + 6
        width: root.width
        padding: 5
        implicitHeight: Math.min(contentItem.implicitHeight + 10, 300)
        contentItem: ListView {
            clip: true
            implicitHeight: contentHeight
            model: root.popup.visible ? root.delegateModel : null
            currentIndex: root.highlightedIndex
            ScrollIndicator.vertical: ScrollIndicator {}
        }
        background: Rectangle {
            radius: Theme.radius
            color: Theme.elevated
            border.color: Theme.border
        }
    }
}
