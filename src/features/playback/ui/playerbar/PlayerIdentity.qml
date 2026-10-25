/**
 * Location: src/features/playback/ui/playerbar/PlayerIdentity.qml
 * Function: Current-track artwork, title, and subtitle for the player bar.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "../../../../shared/ui"

RowLayout {
    id: root
    spacing: 12

    readonly property string coverSource: {
        var c = playbackController.currentTrackCover;
        return (c && (c.indexOf("file://") === 0 ||
                      c.indexOf("http") === 0)) ? c : "";
    }

    Rectangle {
        Layout.preferredWidth: 48
        Layout.preferredHeight: 48
        radius: Theme.radiusSm
        color: Theme.bgSurface
        border.color: Theme.borderDark; border.width: 1

        AppIcon {
            anchors.centerIn: parent
            name: "disc"; size: 22
            iconColor: Theme.textDim
        }
        RoundedImage {
            anchors.fill: parent; radius: Theme.radiusSm
            source: root.coverSource
        }
    }

    ColumnLayout {
        Layout.fillWidth: true; spacing: 2

        Text {
            Layout.fillWidth: true
            text: playbackController.currentTrackTitle.length > 0 ?
                playbackController.currentTrackTitle :
                Strings.get("player_idle_title")
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontBase
            font.bold: true
            color: Theme.textLight
            elide: Text.ElideRight
        }

        Text {
            Layout.fillWidth: true
            text: playbackController.currentTrackArtist.length > 0 ?
                playbackController.currentTrackArtist :
                (playbackController.currentTrackAlbum.length > 0 ?
                 playbackController.currentTrackAlbum :
                 Strings.get("player_hi_fi"))
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            color: Theme.textMuted
            elide: Text.ElideRight
        }
    }
}
