// SPDX-FileCopyrightText: 2025 OpenAI
// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: MIT
// Selected Apps SDK UI tokens adapted to QML. See third_party/apps-sdk-ui/LICENSE.
pragma Singleton
import QtQuick

QtObject {
    property bool dark: true
    readonly property color surface: dark ? "#212121" : "#ffffff"
    readonly property color sidebar: dark ? "#181818" : "#f9f9f9"
    readonly property color elevated: dark ? "#303030" : "#ffffff"
    readonly property color hover: dark ? "#393939" : "#ededed"
    readonly property color text: dark ? "#ffffff" : "#0d0d0d"
    readonly property color secondary: dark ? "#afafaf" : "#5d5d5d"
    readonly property color tertiary: dark ? "#8f8f8f" : "#767676"
    readonly property color border: dark ? "#393939" : "#dfdfdf"
    readonly property color subtleBorder: dark ? "#303030" : "#ededed"
    readonly property color solid: dark ? "#f3f3f3" : "#181818"
    readonly property color solidText: dark ? "#0d0d0d" : "#ffffff"
    readonly property int radius: 12
    readonly property int smallRadius: 8
    readonly property int unit: 4
    readonly property int bodySize: 14
    readonly property int smallSize: 12
    readonly property int controlHeight: 40
}
