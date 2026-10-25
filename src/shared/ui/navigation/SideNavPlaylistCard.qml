/**
 * Location: src/shared/ui/navigation/SideNavPlaylistCard.qml
 * Function: Outlined sidebar action that opens the playlist collection,
 * collapsing to a single plus glyph when the rail is icon-only.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import ".."

Rectangle {
    id: root
    property bool collapsed: false
    implicitHeight: 38
    radius: Theme.radiusFull
    color: hover.containsMouse ? Theme.bgSurface : "transparent"
    border.width: 1; border.color: Theme.borderDark
    Behavior on color { ColorAnimation { duration: 120 } }

    signal newPlaylistRequested()

    AppIcon {
        visible: root.collapsed
        anchors.centerIn: parent
        name: "plus"; size: 16; iconColor: Theme.textLight
    }

    Row {
        visible: !root.collapsed
        anchors.centerIn: parent
        spacing: Theme.spaceSm
        AppIcon { name: "plus"; size: 16; iconColor: Theme.textLight }
        Text {
            text: Strings.get("nav_new_playlist")
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBase
            font.bold: true; color: Theme.textLight
        }
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.newPlaylistRequested()
    }

    ToolTip.visible: root.collapsed && hover.containsMouse
    ToolTip.text: Strings.get("nav_new_playlist")
    ToolTip.delay: 350
}
