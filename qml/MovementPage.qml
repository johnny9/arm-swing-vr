// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Controls.Basic
import QtQuick.Layouts

ColumnLayout {
    id: page
    required property var controller
    spacing: 24
    SettingRow {
        title: "Hold to move"
        description: "Swinging only activates while this input is held."
        InputPicker {
            Layout.fillWidth: true
            currentPath: page.controller.profile.activation_input
            accessibleLabel: "Hold to move"
            onPathSelected: path => page.controller.setField("activation_input", path)
        }
    }
    SettingRow {
        title: "Swing with"
        description: "The arms used to measure your movement."
        AppCombo {
            objectName: "armsCombo"
            Layout.fillWidth: true
            model: [
                {
                    label: "Both arms",
                    value: "both"
                },
                {
                    label: "Left arm",
                    value: "left"
                },
                {
                    label: "Right arm",
                    value: "right"
                }
            ]
            selectedValue: page.controller.profile.contributing_arms
            onActivated: page.controller.setField("contributing_arms", currentValue)
        }
    }
    SettingRow {
        title: "Steer with"
        description: "The direction you look or point sets your heading."
        AppCombo {
            Layout.fillWidth: true
            model: [
                {
                    label: "Head direction",
                    value: "head"
                },
                {
                    label: "Left hand",
                    value: "left-hand"
                },
                {
                    label: "Right hand",
                    value: "right-hand"
                }
            ]
            selectedValue: page.controller.profile.steering
            onActivated: page.controller.setField("steering", currentValue)
        }
    }
    Divider {}
    RowLayout {
        Layout.fillWidth: true
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 6
            Text {
                text: "Sensitivity"
                color: Theme.text
                font.pixelSize: 14
            }
            Text {
                text: "How much movement each swing produces."
                color: Theme.secondary
                font.pixelSize: 12
            }
        }
        Item {
            Layout.fillWidth: true
        }
        Rectangle {
            implicitWidth: 64
            implicitHeight: 36
            radius: 8
            color: Theme.elevated
            border.color: Theme.border
            Text {
                anchors.centerIn: parent
                text: Number(page.controller.profile.sensitivity).toFixed(2) + "×"
                color: Theme.text
                font.pixelSize: 14
            }
        }
    }
    Slider {
        id: sensitivity
        objectName: "sensitivitySlider"
        Layout.fillWidth: true
        Layout.topMargin: -12
        from: 0.1
        to: 5
        stepSize: 0.01
        value: page.controller.profile.sensitivity
        Accessible.name: "Arm swing sensitivity"
        onMoved: page.controller.setField("sensitivity", Math.round(value * 100) / 100)
        background: Rectangle {
            x: sensitivity.leftPadding
            y: sensitivity.topPadding + sensitivity.availableHeight / 2 - height / 2
            width: sensitivity.availableWidth
            height: 4
            radius: 2
            color: Theme.border
            Rectangle {
                width: sensitivity.visualPosition * parent.width
                height: parent.height
                radius: 2
                color: Theme.secondary
            }
        }
        handle: Rectangle {
            x: sensitivity.leftPadding + sensitivity.visualPosition * (sensitivity.availableWidth - width)
            y: sensitivity.topPadding + sensitivity.availableHeight / 2 - height / 2
            width: 20
            height: 20
            radius: 10
            color: Theme.solid
            border.width: sensitivity.activeFocus ? 3 : 0
            border.color: Theme.secondary
        }
    }
    RowLayout {
        Layout.fillWidth: true
        Layout.topMargin: -24
        Text {
            text: "Gentler · 0.1×"
            color: Theme.tertiary
            font.pixelSize: 12
        }
        Item {
            Layout.fillWidth: true
        }
        Text {
            text: "Stronger · 5×"
            color: Theme.tertiary
            font.pixelSize: 12
        }
    }
    Divider {}
    SettingRow {
        title: "Movement controller"
        description: "The stick input your game uses for walking."
        AppCombo {
            objectName: "outputCombo"
            Layout.fillWidth: true
            model: [
                {
                    label: "Left controller",
                    value: "left"
                },
                {
                    label: "Right controller",
                    value: "right"
                }
            ]
            selectedValue: page.controller.profile.output_hand
            onActivated: page.controller.setField("output_hand", currentValue)
        }
    }
    Text {
        Layout.fillWidth: true
        text: "Activation, measured arms, and movement controller can be set independently."
        color: Theme.tertiary
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }
}
