/**
 * Location: src/features/playback/ui/PlayerBar.qml
 * Function: Compact 76px bottom playback bar providing universal transport,
 * track progress, volume control, and stream quality pill across views.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"

Rectangle {
    id: root; height: 76; color: Theme.bgCard; radius: Theme.radiusMd
    clip: true; border.color: Theme.borderDark; border.width: 1

    RowLayout {
        anchors.fill: parent; spacing: Theme.spaceMd
        anchors.margins: Theme.spaceMd

        RowLayout {
            Layout.preferredWidth: 240; spacing: Theme.spaceSm
            Rectangle {
                width: 46; height: 46; radius: Theme.radiusSm
                color: Theme.bgSurface; clip: true
                Image {
                    anchors.fill: parent; fillMode: Image.PreserveAspectCrop
                    source: {
                        var c = playbackController.currentTrackCover;
                        return (c && (c.indexOf("file://") === 0 ||
                                      c.indexOf("http") === 0)) ? c : "";
                    }
                    visible: status === Image.Ready &&
                             source.toString().length > 0
                }
                AppIcon {
                    anchors.centerIn: parent; name: "disc"; size: 22
                    iconColor: Theme.textDim
                    visible: !parent.children[0].visible
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
                    text: playbackController.currentTrackArtist +
                        (playbackController.currentStationName.length > 0 ?
                            " · " + playbackController.currentStationName : "")
                    font.family: Theme.fontFamily; font.pixelSize: 10
                    color: Theme.textMuted; elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        PlayerBarControls {
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.preferredWidth: 220; spacing: Theme.spaceSm
            AppIcon { name: "volume"; size: 16; iconColor: Theme.textMuted }
            Slider {
                Layout.preferredWidth: 80; from: 0; to: 100; stepSize: 1
                value: playbackController.volume
                onMoved: playbackController.setVolume(Math.round(value))
            }
            Rectangle {
                implicitHeight: 22; implicitWidth: 100; radius: Theme.radiusSm
                color: Theme.bgSurface; border.color: Theme.borderDark
                border.width: 1
                visible: playbackController.currentTitle !== ""
                Text {
                    anchors.centerIn: parent; font.bold: true
                    text: playbackController.streamQuality
                    font.family: Theme.fontFamily; font.pixelSize: 10
                    color: Theme.textMuted
                }
            }
        }
    }
}
