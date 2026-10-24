/**
 * Location: src/features/library/ui/LibraryTrackDelegate.qml
 * Function: Reusable track row rendering title, artist, duration, double-click
 * playback dispatch, and the Android-style "Play mix" action.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "../../../shared/ui"

Rectangle {
    id: root
    width: parent.width; height: 50
    color: hover.containsMouse ? Theme.bgSurface : "transparent"
    radius: Theme.radiusSm

    property var track: modelData
    signal playRequested(var t)
    signal mixRequested(var t)

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onDoubleClicked: root.playRequested(root.track)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceSm
        spacing: Theme.spaceMd

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 1
            Text {
                text: root.track.titulo || root.track.title || ""
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontMd
                color: Theme.textLight
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                text: root.track.artista || root.track.artist || ""
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSm
                color: Theme.textMuted
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }

        Text {
            text: Theme.formatDuration(
                root.track.duracion || root.track.duration || 0)
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            color: Theme.textDim
            Layout.preferredWidth: 48
            horizontalAlignment: Text.AlignRight
        }

        AppButton {
            text: Strings.get("action_play_mix")
            variant: "secondary"
            onClicked: root.mixRequested(root.track)
        }
    }
}
