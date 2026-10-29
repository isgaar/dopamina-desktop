/**
 * Location: src/features/playback/ui/playerbar/PlayerIdentity.qml
 * Function: Current-track artwork, title, and subtitle for the player bar.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "../../../../shared/ui"
import "../../../library/ui"

RowLayout {
    id: root
    spacing: 12

    readonly property string coverSource: {
        var c = playbackController.currentTrackCover;
        return (c && (c.indexOf("file://") === 0 ||
                      c.indexOf("http") === 0)) ? c : "";
    }
    readonly property var activeTrackData: {
        var tid = playbackController ? playbackController.currentTrackId : 0;
        if (tid > 0 && typeof libraryController !== "undefined" &&
            libraryController) {
            var found = libraryController.findTrackById(tid);
            if (found && found.id) return found;
        }
        if (playbackController &&
            playbackController.currentTrackTitle.length > 0) {
            return {
                id: tid,
                title: playbackController.currentTrackTitle,
                artist: playbackController.currentTrackArtist,
                album: playbackController.currentTrackAlbum,
                fidelityTier: "MAX",
                qualityDetails: "FLAC 24-bit / 44 kHz",
                extraMeta: ""
            };
        }
        return null;
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

        RowLayout {
            Layout.fillWidth: true
            spacing: 6

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

            AudioQualityBadge {
                visible: root.activeTrackData !== null
                track: root.activeTrackData
            }
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

        PlayerSessionSource {
            Layout.fillWidth: true
        }
    }
}
