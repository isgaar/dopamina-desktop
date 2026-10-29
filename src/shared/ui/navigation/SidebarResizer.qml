/**
 * Location: src/shared/ui/navigation/SidebarResizer.qml
 * Function: Interactive drag handle allowing smooth resizing and collapsing
 * of the navigation rail with visual highlight feedback on hover.
 * All Rights Reserved isgaar
 */
import QtQuick
import "root:/src/shared"

Rectangle {
    id: root
    property var sideNav: null
    property var settingsController: null
    property real maxContainerWidth: 1000

    width: 7; z: 20
    color: "transparent"

    Rectangle {
        anchors.centerIn: parent; width: 1; height: parent.height
        color: resizerM.containsMouse || resizerM.isDragging ?
            Theme.primary : Theme.withAlpha(Theme.borderDark, 0.65)
        Behavior on color { ColorAnimation { duration: 120 } }
    }

    MouseArea {
        id: resizerM; anchors.fill: parent
        hoverEnabled: true; cursorShape: Qt.SplitHCursor
        property bool isDragging: false

        onPressed: mouse => {
            isDragging = true
            if (root.sideNav) root.sideNav.animating = false
        }
        onPositionChanged: mouse => {
            if (!isDragging || !root.sideNav || !root.parent) return
            var globalX = mapToItem(root.parent, mouse.x, 0).x
            if (globalX < 110) {
                root.sideNav.collapsed = true
            } else {
                var maxW = Math.min(
                    450, Math.floor(root.maxContainerWidth * 0.40))
                var cl = Math.max(160, Math.min(Math.round(globalX), maxW))
                root.sideNav.collapsed = false
                root.sideNav.targetWidth = cl
                root.sideNav.lastExpandedWidth = cl
            }
        }
        onReleased: {
            isDragging = false
            if (root.sideNav) root.sideNav.animating = true
            if (root.settingsController && root.sideNav) {
                root.settingsController.sidebarWidth = root.sideNav.targetWidth
                root.settingsController.sidebarCollapsed =
                    root.sideNav.collapsed
            }
        }
        onDoubleClicked: {
            if (!root.sideNav) return
            root.sideNav.animating = true
            if (root.sideNav.collapsed) {
                root.sideNav.collapsed = false
                root.sideNav.targetWidth = root.sideNav.lastExpandedWidth || 240
            } else {
                root.sideNav.lastExpandedWidth = root.sideNav.targetWidth || 240
                root.sideNav.collapsed = true
            }
            if (root.settingsController) {
                root.settingsController.sidebarWidth = root.sideNav.targetWidth
                root.settingsController.sidebarCollapsed =
                    root.sideNav.collapsed
            }
        }
    }
}
