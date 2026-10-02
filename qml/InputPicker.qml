// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: root
    property string currentPath: ""
    property bool custom: false
    property bool editingPath: false
    property string accessibleLabel: "Controller input"
    readonly property var choices: [
        {
            label: "Choose an input",
            value: ""
        },
        {
            label: "Left · A button",
            value: "/user/hand/left/input/a/click"
        },
        {
            label: "Left · B button",
            value: "/user/hand/left/input/b/click"
        },
        {
            label: "Right · A button",
            value: "/user/hand/right/input/a/click"
        },
        {
            label: "Right · B button",
            value: "/user/hand/right/input/b/click"
        },
        {
            label: "Custom input…",
            value: "custom"
        }
    ]
    signal pathSelected(string path)
    function indexForPath() {
        for (let i = 0; i < choices.length - 1; ++i)
            if (choices[i].value === currentPath)
                return i;
        return choices.length - 1;
    }
    onCurrentPathChanged: {
        if (!editingPath)
            custom = indexForPath() === choices.length - 1;
    }
    Component.onCompleted: custom = indexForPath() === choices.length - 1
    spacing: 8
    AppCombo {
        objectName: "inputPickerCombo"
        Layout.fillWidth: true
        model: root.choices
        selectedValue: root.choices[root.custom ? root.choices.length - 1 : root.indexForPath()].value
        Accessible.name: root.accessibleLabel
        onActivated: {
            if (currentValue === "custom")
                root.custom = true;
            else {
                root.custom = false;
                root.pathSelected(currentValue);
            }
        }
    }
    AppField {
        objectName: "customInputPath"
        Layout.fillWidth: true
        visible: root.custom
        text: root.currentPath
        placeholderText: "Controller input path"
        Accessible.name: root.accessibleLabel + " path"
        onTextEdited: {
            // Clearing a custom path must not hide the field during editing.
            root.editingPath = true;
            root.pathSelected(text);
            root.editingPath = false;
        }
    }
}
