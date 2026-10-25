/**
 * Location: src/shared/ui/navigation/SideNavLikedCard.qml
 * Function: Liked-track shortcut that shrinks to a single star badge when
 * the rail is icon-only, hiding the playlist hint.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import ".."

Item {
    id: root
    property bool collapsed: false
    implicitHeight: column.implicitHeight
    signal likedMusicRequested()

    Column {
        id: column
        anchors.fill: parent; spacing: Theme.spaceSm

        Rectangle {
            id: liked
            width: column.width; height: root.collapsed ? 44 : 54
            radius: Theme.radiusMd
            color: hover.containsMouse ? Theme.bgSurface :
                Theme.withAlpha(Theme.bgSurface, 0.55)
            border.color: Theme.borderDark; border.width: 1

            Rectangle {
                id: badge
                width: 30; height: 30
                x: root.collapsed ? (liked.width - 30) / 2 : Theme.spaceSm
                anchors.verticalCenter: parent.verticalCenter
                radius: Theme.radiusSm
                color: Theme.primaryDim
                AppIcon {
                    anchors.centerIn: parent; name: "star"; size: 15
                    iconColor: Theme.primaryHover
                }            }

            Column {
                visible: !root.collapsed
                x: badge.x + badge.width + Theme.spaceSm
                width: liked.width - x - Theme.spaceXl
                anchors.verticalCenter: parent.verticalCenter; spacing: 1
                Text {
                    width: parent.width; elide: Text.ElideRight
                    text: Strings.get("nav_liked_title")
                    font.family: Theme.fontFamily; font.pixelSize: 12
                    font.bold: true; color: Theme.textLight
                }
                Text {
                    width: parent.width; elide: Text.ElideRight
                    text: Strings.get("nav_liked_subtitle")
                    font.family: Theme.fontFamily; font.pixelSize: 10
                    color: Theme.textMuted
                }
            }

            MouseArea {
                id: hover
                anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.likedMusicRequested()
            }

            ToolTip.visible: root.collapsed && hover.containsMouse
            ToolTip.text: Strings.get("nav_liked_title")
            ToolTip.delay: 350
        }

        Text {
            visible: !root.collapsed; width: column.width
            text: Strings.get("nav_playlists_hint")
            font.family: Theme.fontFamily; font.pixelSize: 11
            color: Theme.textDim
            wrapMode: Text.WordWrap; lineHeight: 1.3
        }
    }
}
