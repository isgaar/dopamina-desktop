/**
 * Location: src/features/playlists/ui/PlaylistRow.qml
 * Function: Playlist collection row with icon, name, hover feedback, and
 * delete action.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "../../../shared/ui"

Rectangle {
    id: root
    property var entry: null
    signal deleteRequested(var entry)

    width: ListView.view ? ListView.view.width : 0
    height: 50
    color: hover.containsMouse ? Theme.bgSurface : "transparent"
    radius: Theme.radiusSm

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        acceptedButtons: Qt.NoButton
    }

    RowLayout {
        anchors.fill: parent; anchors.margins: Theme.spaceSm
        spacing: Theme.spaceMd
        AppIcon {
            name: "list-music"; size: 20
            iconColor: Theme.primaryHover
        }
        Text {
            text: root.entry ? (root.entry.nombre || "") : ""
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontMd
            color: Theme.textLight
            Layout.fillWidth: true; elide: Text.ElideRight
        }
        AppButton {
            text: Strings.get("btn_delete")
            variant: "secondary"
            onClicked: root.deleteRequested(root.entry)
        }
    }
}
