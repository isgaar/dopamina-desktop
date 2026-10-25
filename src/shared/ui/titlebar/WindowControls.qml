/**
 * Location: src/shared/ui/titlebar/WindowControls.qml
 * Function: Native window caption controls providing minimize, maximize,
 * restore, and close buttons flush to the corner for desktop environments.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Window

Row {
    id: root
    property var window: null
    property bool canMaximize: true
    anchors.top: parent ? parent.top : undefined
    anchors.bottom: parent ? parent.bottom : undefined
    height: parent ? parent.height : 56
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
            width: 46; height: root.height; opacity: btnEnabled ? 1.0 : 0.35
            topRightRadius: isClose && window && window.isFloat
                ? Theme.radiusMd : 0
            color: btnM.containsMouse && btnEnabled
                ? (isClose ? Theme.danger : Theme.bgSurface) : "transparent"
            Behavior on color { ColorAnimation { duration: 100 } }

            Item {
                anchors.centerIn: parent; width: 10; height: 10
                Rectangle {
                    visible: modelData.id === "min"; anchors.centerIn: parent
                    width: 10; height: 1; color: Theme.textLight
                }
                Rectangle {
                    visible: isMaxBtn && !root.isMax; anchors.fill: parent
                    color: "transparent"; border.color: Theme.textLight
                    border.width: 1
                }
                Item {
                    visible: isMaxBtn && root.isMax; anchors.fill: parent
                    Rectangle {
                        x: 2; y: 0; width: 7; height: 1
                        color: Theme.textLight
                    }
                    Rectangle {
                        x: 8; y: 1; width: 1; height: 7
                        color: Theme.textLight
                    }
                    Rectangle {
                        x: 0; y: 2; width: 8; height: 8; color: "transparent"
                        border.color: Theme.textLight; border.width: 1
                    }
                }
                Text {
                    visible: isClose; anchors.centerIn: parent
                    text: "\u2715"; font.pixelSize: 11
                    font.family: Theme.fontFamily
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
