// SPDX-FileCopyrightText: 2026 Arm Swing VR contributors
// SPDX-License-Identifier: GPL-3.0-or-later
import QtQuick

Canvas {
    id: root
    property string name: "profile"
    property color stroke: Theme.text
    implicitWidth: 20
    implicitHeight: 20
    onNameChanged: requestPaint()
    onStrokeChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()
    onPaint: {
        const c = getContext("2d");
        c.reset();
        c.scale(width / 24, height / 24);
        c.strokeStyle = stroke;
        c.lineWidth = 1.6;
        c.lineCap = "round";
        c.lineJoin = "round";
        c.beginPath();
        if (name === "plus") {
            c.moveTo(12, 5);
            c.lineTo(12, 19);
            c.moveTo(5, 12);
            c.lineTo(19, 12);
        } else if (name === "folder") {
            c.moveTo(3, 7);
            c.lineTo(3, 19);
            c.lineTo(21, 19);
            c.lineTo(21, 8);
            c.lineTo(12, 8);
            c.lineTo(10, 5);
            c.lineTo(3, 5);
            c.lineTo(3, 7);
        } else if (name === "motion") {
            c.moveTo(2, 14);
            c.bezierCurveTo(6, 14, 5, 5, 9, 5);
            c.bezierCurveTo(13, 5, 10, 19, 15, 19);
            c.bezierCurveTo(19, 19, 17, 10, 22, 10);
        } else if (name === "mapping") {
            c.moveTo(4, 7);
            c.lineTo(20, 7);
            c.lineTo(16, 3);
            c.moveTo(20, 7);
            c.lineTo(16, 11);
            c.moveTo(20, 17);
            c.lineTo(4, 17);
            c.lineTo(8, 13);
            c.moveTo(4, 17);
            c.lineTo(8, 21);
        } else if (name === "chevron") {
            c.moveTo(7, 10);
            c.lineTo(12, 15);
            c.lineTo(17, 10);
        } else if (name === "arrow") {
            c.moveTo(4, 12);
            c.lineTo(20, 12);
            c.moveTo(15, 7);
            c.lineTo(20, 12);
            c.lineTo(15, 17);
        } else if (name === "check") {
            c.moveTo(5, 12);
            c.lineTo(10, 17);
            c.lineTo(20, 6);
        } else if (name === "close") {
            c.moveTo(7, 7);
            c.lineTo(17, 17);
            c.moveTo(17, 7);
            c.lineTo(7, 17);
        } else if (name === "copy") {
            c.rect(8, 8, 12, 12);
            c.moveTo(16, 8);
            c.lineTo(16, 4);
            c.lineTo(4, 4);
            c.lineTo(4, 16);
            c.lineTo(8, 16);
        } else if (name === "sun") {
            c.arc(12, 12, 4, 0, Math.PI * 2);
            for (let i = 0; i < 8; i++) {
                const a = i * Math.PI / 4;
                c.moveTo(12 + 7 * Math.cos(a), 12 + 7 * Math.sin(a));
                c.lineTo(12 + 9 * Math.cos(a), 12 + 9 * Math.sin(a));
            }
        } else if (name === "moon") {
            c.moveTo(16, 3);
            c.bezierCurveTo(1, 0, 0, 22, 15, 21);
            c.bezierCurveTo(20, 21, 22, 16, 21, 13);
            c.bezierCurveTo(13, 18, 8, 8, 16, 3);
        } else {
            c.moveTo(6, 3);
            c.lineTo(16, 3);
            c.lineTo(20, 7);
            c.lineTo(20, 21);
            c.lineTo(6, 21);
            c.closePath();
            c.moveTo(10, 10);
            c.lineTo(16, 10);
            c.moveTo(10, 14);
            c.lineTo(16, 14);
        }
        c.stroke();
    }
}
