/**
 * Location: src/features/library/ui/LibraryTrackDelegate.qml
 * Function: Reusable track row delegate rendering title, artist, duration,
 * double-click playback dispatching, and Oír radio action trigger button.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "../../../shared/ui"

Rectangle {
    id: root
    width: parent.width; height: 50
    color: ma.containsMouse ? Theme.bgSurface : "transparent"
    radius: Theme.radiusSm

    property var track: modelData
    signal playRequested(var t)
    signal radioRequested(var t)

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceSm
        spacing: Theme.spaceMd

        Text {
            text: root.track.titulo || root.track.title || ""
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontMd
            color: Theme.textLight
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Text {
            text: root.track.artista || root.track.artist || ""
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            color: Theme.textMuted
            Layout.preferredWidth: 160
            elide: Text.ElideRight
        }

        AppButton {
            text: Strings.get("action_play_radio")
            onClicked: root.radioRequested(root.track)
        }
    }

    MouseArea {
        id: ma
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onDoubleClicked: root.playRequested(root.track)
    }
}
