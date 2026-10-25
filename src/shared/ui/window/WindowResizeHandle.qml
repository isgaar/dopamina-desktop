/**
 * Location: src/shared/ui/window/WindowResizeHandle.qml
 * Function: Single frameless-window edge grip dispatching a system resize
 * request for the given Qt edges.
 * All Rights Reserved isgaar
 */
import QtQuick

Item {
    id: root
    property var target: null
    property int edges: Qt.LeftEdge
    property int pointer: Qt.ArrowCursor
    property bool active: true

    MouseArea {
        anchors.fill: parent
        enabled: root.active
        cursorShape: root.pointer
        onPressed: if (root.active && root.target)
            root.target.startSystemResize(root.edges)
    }
}
