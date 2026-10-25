/**
 * Location: src/shared/ui/window/WindowResizeHandles.qml
 * Function: Eight transparent grips restoring native edge and corner resize
 * behavior for frameless desktop windows.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Window

Item {
    id: root
    property var window: null
    property int grip: 6
    z: 1000

    readonly property bool active: window !== null &&
        Platform.useCustomTitleBar &&
        window.visibility !== Window.Maximized

    WindowResizeHandle {
        target: root.window; active: root.active
        edges: Qt.LeftEdge; pointer: Qt.SizeHorCursor
        x: 0; y: root.grip
        width: root.grip; height: parent.height - root.grip * 2
    }
    WindowResizeHandle {
        target: root.window; active: root.active
        edges: Qt.RightEdge; pointer: Qt.SizeHorCursor
        x: parent.width - root.grip; y: root.grip
        width: root.grip; height: parent.height - root.grip * 2
    }
    WindowResizeHandle {
        target: root.window; active: root.active
        edges: Qt.TopEdge; pointer: Qt.SizeVerCursor
        x: root.grip; y: 0
        width: parent.width - root.grip * 2; height: root.grip
    }
    WindowResizeHandle {
        target: root.window; active: root.active
        edges: Qt.BottomEdge; pointer: Qt.SizeVerCursor
        x: root.grip; y: parent.height - root.grip
        width: parent.width - root.grip * 2; height: root.grip
    }
    WindowResizeHandle {
        target: root.window; active: root.active
        edges: Qt.LeftEdge | Qt.TopEdge; pointer: Qt.SizeFDiagCursor
        width: root.grip; height: root.grip
    }
    WindowResizeHandle {
        target: root.window; active: root.active
        edges: Qt.RightEdge | Qt.TopEdge; pointer: Qt.SizeBDiagCursor
        x: parent.width - root.grip; width: root.grip; height: root.grip
    }
    WindowResizeHandle {
        target: root.window; active: root.active
        edges: Qt.LeftEdge | Qt.BottomEdge; pointer: Qt.SizeBDiagCursor
        y: parent.height - root.grip
        width: root.grip; height: root.grip
    }
    WindowResizeHandle {
        target: root.window; active: root.active
        edges: Qt.RightEdge | Qt.BottomEdge; pointer: Qt.SizeFDiagCursor
        x: parent.width - root.grip; y: parent.height - root.grip
        width: root.grip; height: root.grip
    }
}
