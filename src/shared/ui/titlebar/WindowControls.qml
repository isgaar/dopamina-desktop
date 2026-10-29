/**
 * Location: src/shared/ui/titlebar/WindowControls.qml
 * Function: Native window caption controls providing minimize, maximize,
 * restore, and close buttons flush to the corner for desktop environments.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Window
import "root:/src/shared"

Row {
    id: root
    property var window: null
    property bool canMaximize: true
    spacing: 6
    height: 32

    readonly property bool isMax: window &&
        window.visibility === Window.Maximized

    function doAction(act) {
        if (!window) return
        if (act === "min") window.showMinimized()
        else if (act === "max" && canMaximize)
            isMax ? window.showNormal() : window.showMaximized()
        else if (act === "close") window.close()
    }

    Repeater {
        model: [{ id: "min" }, { id: "max" }, { id: "close", isClose: true }]

        delegate: Rectangle {
            id: btn
            readonly property bool isMaxBtn: modelData.id === "max"
            readonly property bool isClose: modelData.isClose === true
            readonly property bool btnEnabled: !isMaxBtn || root.canMaximize
            width: 32; height: 32; radius: 16
            opacity: btnEnabled ? 1.0 : 0.35
            color: btnM.containsMouse && btnEnabled
                ? (isClose ? Theme.danger : Theme.bgSurface) : "transparent"
            Behavior on color { ColorAnimation { duration: 120 } }

            Item {
                anchors.centerIn: parent; width: 12; height: 12
                Rectangle {
                    visible: modelData.id === "min"; anchors.centerIn: parent
                    width: 10; height: 1.5; radius: 1; color: Theme.textLight
                }
                Rectangle {
                    visible: isMaxBtn && !root.isMax; anchors.centerIn: parent
                    width: 10; height: 10; radius: 2
                    color: "transparent"; border.color: Theme.textLight
                    border.width: 1.2
                }
                Item {
                    visible: isMaxBtn && root.isMax; anchors.fill: parent
                    Rectangle {
                        x: 3; y: 1; width: 7; height: 1.2; radius: 1
                        color: Theme.textLight
                    }
                    Rectangle {
                        x: 9; y: 2; width: 1.2; height: 7; radius: 1
                        color: Theme.textLight
                    }
                    Rectangle {
                        x: 1; y: 3; width: 8; height: 8; radius: 2
                        color: "transparent"
                        border.color: Theme.textLight; border.width: 1.2
                    }
                }
                Text {
                    visible: isClose; anchors.centerIn: parent
                    text: "\u2715"; font.pixelSize: 12
                    font.family: Theme.fontFamily
                    font.bold: true
                    color: (isClose && btnM.containsMouse)
                        ? "#FFFFFF" : Theme.textLight
                }
            }

            MouseArea {
                id: btnM; anchors.fill: parent
                enabled: btn.btnEnabled; hoverEnabled: btn.btnEnabled
                cursorShape: btn.btnEnabled
                    ? Qt.PointingHandCursor : Qt.ArrowCursor
                onClicked: root.doAction(modelData.id)
            }
        }
    }
}

