// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick
import QtQuick.Layouts

ColumnLayout {
    id: page
    required property var controller
    spacing: 24
    Text {
        text: "Game setup"
        color: Theme.text
        font.pixelSize: 22
        font.weight: Font.DemiBold
    }
    Text {
        Layout.fillWidth: true
        text: "Give each game its own movement and controller settings."
        color: Theme.secondary
        font.pixelSize: 14
        wrapMode: Text.WordWrap
        Layout.topMargin: -16
    }
    Divider {}
    SettingRow {
        title: "Profile name"
        description: "A game name or a setup you recognize."
        AppField {
            objectName: "profileNameField"
            Layout.fillWidth: true
            text: page.controller.profile.name
            Accessible.name: "Profile name"
            onTextEdited: page.controller.setField("name", text)
        }
    }
    SettingRow {
        title: "Steam App ID"
        description: "Optional. Leave blank for a non-Steam game."
        AppField {
            Layout.fillWidth: true
            text: page.controller.profile.steam_app_id
            placeholderText: "e.g. 658920"
            Accessible.name: "Steam App ID"
            onTextEdited: page.controller.setField("steam_app_id", text)
        }
    }
    Divider {}
    SettingRow {
        title: "VR runtime"
        description: "The API this game uses to receive input."
        AppCombo {
            Layout.fillWidth: true
            model: [
                {
                    label: "SteamVR / OpenVR",
                    value: "openvr"
                },
                {
                    label: "OpenXR",
                    value: "openxr"
                }
            ]
            selectedValue: page.controller.profile.runtime
            onActivated: page.controller.setField("runtime", currentValue)
        }
    }
    SettingRow {
        title: "Controller profile"
        description: "Knuckles identifies Valve Index controllers."
        AppField {
            Layout.fillWidth: true
            text: page.controller.profile.controller
            Accessible.name: "Controller profile"
            onTextEdited: page.controller.setField("controller", text)
        }
    }
    Text {
        Layout.fillWidth: true
        text: "These choices are saved as configuration. Controller support and game compatibility have not yet been validated."
        color: Theme.tertiary
        font.pixelSize: 12
        wrapMode: Text.WordWrap
    }
}
