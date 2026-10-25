/**
 * Location: src/features/playback/ui/playerbar/TransportButton.qml
 * Function: Circular transport icon button with hover surface and active
 * accent coloring for shuffle and repeat modes.
 * All Rights Reserved isgaar
 */
import QtQuick
import "../../../../shared/ui"

Item {
    id: root
    property string iconName: ""
    property bool active: false
    property int iconSize: 16
    signal clicked()

    implicitWidth: 30
    implicitHeight: 30

    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: hover.containsMouse ? Theme.bgSurface : "transparent"

        AppIcon {
            anchors.centerIn: parent
            name: root.iconName
            size: root.iconSize
            iconColor: root.active ? Theme.textLight : Theme.textMuted
        }
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
