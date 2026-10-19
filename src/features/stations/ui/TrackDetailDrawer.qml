/**
 * Location: src/features/stations/ui/TrackDetailDrawer.qml
 * Function: Collapsible track metadata inspector displaying full file paths,
 * bitrate, BPM rhythm metrics, and quick station reassignment dropdowns.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"

Rectangle {
    id: root
    height: 84; color: "transparent"

    property var controller: null
    signal changeStationRequested(int trackId)

    Rectangle {
        anchors.top: parent.top; width: parent.width; height: 1
        color: Theme.borderDark
    }

    RowLayout {
        anchors.fill: parent; anchors.margins: Theme.spaceMd
        spacing: Theme.spaceLg

        Rectangle {
            width: 52; height: 52; radius: Theme.radiusSm
            color: Theme.bgSurface; border.color: Theme.primary; border.width: 1
            AppIcon {
                anchors.centerIn: parent; name: "disc-album"
                size: 24; iconColor: Theme.primary
            }
        }

        ColumnLayout {
            Layout.fillWidth: true; Layout.preferredWidth: 3; spacing: 2
            Text {
                text: root.controller && root.controller.inspectedTitle !== ""
                    ? root.controller.inspectedTitle : "-"
                color: Theme.textLight; font.bold: true
                font.pixelSize: 13; font.family: Theme.fontFamily
                elide: Text.ElideRight; Layout.fillWidth: true
            }
            Text {
                text: (root.controller && root.controller.inspectedArtist !== ""
                        ? root.controller.inspectedArtist : "-") + " • " +
                      (root.controller && root.controller.inspectedAlbum !== ""
                        ? root.controller.inspectedAlbum : "-")
                color: Theme.textMuted; font.pixelSize: 11
                font.family: Theme.fontFamily; elide: Text.ElideRight
                Layout.fillWidth: true
            }
            Text {
                text: root.controller ? root.controller.inspectedPath : ""
                color: Theme.textDim; font.pixelSize: 10
                font.family: Theme.fontFamily; elide: Text.ElideMiddle
                Layout.fillWidth: true
            }
        }
        ColumnLayout {
            Layout.preferredWidth: 160; spacing: 3
            RowLayout {
                spacing: 6
                Text {
                    text: Strings.get("drawer_bpm")
                    color: Theme.textDim; font.pixelSize: 10
                    font.family: Theme.fontFamily
                }
                Text {
                    text: root.controller && root.controller.inspectedBpm > 0
                        ? root.controller.inspectedBpm.toFixed(1) : "-"
                    color: Theme.accent; font.pixelSize: 10; font.bold: true
                    font.family: Theme.fontFamily
                }
            }
            RowLayout {
                spacing: 6
                Text {
                    text: Strings.get("drawer_bitrate")
                    color: Theme.textDim; font.pixelSize: 10
                    font.family: Theme.fontFamily
                }
                Text {
                    text: root.controller &&
                        root.controller.inspectedBitrate > 0
                        ? root.controller.inspectedBitrate + " kbps" : "-"
                    color: Theme.textLight; font.pixelSize: 10
                    font.family: Theme.fontFamily
                }
            }
            RowLayout {
                spacing: 6
                Text {
                    text: Strings.get("table_col_duration")
                    color: Theme.textDim; font.pixelSize: 10
                    font.family: Theme.fontFamily
                }
                Text {
                    text: root.controller
                        ? root.controller.inspectedDuration : ""
                    color: Theme.textLight; font.pixelSize: 10
                    font.family: Theme.fontFamily
                }
            }
        }

        AppButton {
            text: Strings.get("btn_reassign")
            iconName: "shuffle"; variant: "secondary"
            enabled: root.controller && root.controller.inspectedTrackId > 0
            onClicked: if (root.controller &&
                           root.controller.inspectedTrackId > 0) {
                root.changeStationRequested(root.controller.inspectedTrackId)
            }
        }
    }
}
