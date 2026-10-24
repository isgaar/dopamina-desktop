/**
 * Location: src/features/playback/ui/PlayerBar.qml
 * Function: Compact bottom playback bar with artwork, track identity,
 * transport controls, progress, and a reactive volume slider.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"

Rectangle {
    id: root; height: 76
    color: Theme.bgCard; radius: Theme.radiusMd
    border.color: Theme.borderDark; border.width: 1

    readonly property string coverSource: {
        var c = playbackController.currentTrackCover;
        return (c && (c.indexOf("file://") === 0 ||
                      c.indexOf("http") === 0)) ? c : "";
    }
    RowLayout {
        anchors.fill: parent; spacing: Theme.spaceMd
        anchors.margins: Theme.spaceMd

        RowLayout {
            Layout.preferredWidth: 260; spacing: Theme.spaceSm
            Rectangle {
                width: 46; height: 46; radius: Theme.radiusSm
                color: Theme.bgSurface; border.color: Theme.borderDark
                border.width: 1
                AppIcon {
                    anchors.centerIn: parent; name: "disc"; size: 22
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
                    text: playbackController.currentTrackTitle.length > 0 ?
                        playbackController.currentTrackTitle :
                        Strings.get("status_no_playback")
                    font.family: Theme.fontFamily; font.pixelSize: Theme.fontSm
                    font.bold: true; color: Theme.textLight
                    elide: Text.ElideRight; Layout.fillWidth: true
                }
                Text {
                    text: playbackController.currentTrackArtist.length > 0 ?
                        playbackController.currentTrackArtist :
                        playbackController.currentTrackAlbum
                    font.family: Theme.fontFamily; font.pixelSize: 10
                    color: Theme.textMuted; elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        PlayerBarControls { Layout.fillWidth: true }

        RowLayout {
            Layout.preferredWidth: 200; spacing: Theme.spaceSm
            AppIcon { name: "volume"; size: 16; iconColor: Theme.textMuted }
            Slider {
                Layout.fillWidth: true; from: 0; to: 100; stepSize: 1
                value: playbackController.volume
                onMoved: playbackController.setVolume(Math.round(value))
            }
            Text {
                text: playbackController.volume
                font.family: Theme.fontFamily; font.pixelSize: 10
                color: Theme.textMuted; Layout.preferredWidth: 22
                horizontalAlignment: Text.AlignRight
            }
        }
    }
}
