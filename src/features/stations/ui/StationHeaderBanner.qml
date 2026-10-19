/**
 * Location: src/features/stations/ui/StationHeaderBanner.qml
 * Function: Top hero banner displaying station profiles, transmission dial,
 * DJ mode status, aggregate playtime, and direct VLC streaming trigger.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"
Rectangle {
    id: root
    height: 84; color: Theme.bgCard
    border.color: Theme.borderDark; border.width: 1
    property var controller: null
    readonly property bool isLive: root.controller && root.controller.isPlaying
    RowLayout {
        anchors.fill: parent; anchors.margins: Theme.spaceMd
        spacing: Theme.spaceMd
        Rectangle {
            width: 48; height: 48; radius: Theme.radiusMd
            color: Theme.bgSurface; border.width: 1
            border.color: root.isLive ? Theme.danger : Theme.primary
            AppIcon {
                anchors.centerIn: parent; name: "radio"; size: 22
                iconColor: root.isLive ? Theme.danger : Theme.primary
            }
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 2
            RowLayout {
                spacing: Theme.spaceSm
                Text {
                    text: root.controller ?
                        root.controller.currentStationName : ""
                    color: Theme.textLight; font.bold: true; font.pixelSize: 16
                    font.family: Theme.fontFamily
                }
                Rectangle {
                    visible: root.controller &&
                        root.controller.currentStationDial !== ""
                    width: dialB.implicitWidth + 8; height: 18
                    radius: Theme.radiusSm; color: Theme.primaryDim
                    border.color: Theme.primary; border.width: 1
                    Text {
                        id: dialB; anchors.centerIn: parent
                        text: root.controller ?
                            root.controller.currentStationDial : ""
                        color: Theme.primary; font.bold: true
                        font.pixelSize: 10
                    }
                }
                Rectangle {
                    visible: root.controller &&
                        root.controller.currentStationDjMode
                    width: djB.implicitWidth + 8; height: 18
                    radius: Theme.radiusSm; color: Theme.bgSurface
                    border.color: Theme.accent; border.width: 1
                    Text {
                        id: djB; anchors.centerIn: parent; color: Theme.accent
                        text: Strings.get("banner_dj_mode"); font.pixelSize: 10
                        font.bold: true
                    }
                }
                Rectangle {
                    visible: root.isLive
                    width: onAirT.implicitWidth + 8; height: 18
                    radius: Theme.radiusSm; color: Theme.danger
                    Text {
                        id: onAirT; anchors.centerIn: parent
                        text: "● " + Strings.get("banner_on_air")
                        color: Theme.textLight; font.bold: true
                        font.pixelSize: 10
                    }
                }
            }
            Text {
                text: root.controller ? root.controller.currentStationDesc : ""
                color: Theme.textMuted; font.pixelSize: 11
                font.family: Theme.fontFamily; Layout.fillWidth: true
                elide: Text.ElideRight
            }
            RowLayout {
                spacing: 6
                AppIcon { name: "music"; size: 11; iconColor: Theme.textDim }
                Text {
                    color: Theme.textDim; font.pixelSize: 11
                    text: Strings.get("banner_total_tracks") + (root.controller
                        ? root.controller.currentStationTrackCount : 0)
                }
                Item { width: Theme.spaceSm }
                AppIcon { name: "clock"; size: 11; iconColor: Theme.textDim }
                Text {
                    color: Theme.textDim; font.pixelSize: 11
                    text: Strings.get("banner_duration_prefix") +
                        (root.controller ?
                         root.controller.currentStationDuration : "")
                }
            }
        }
        RowLayout {
            spacing: Theme.spaceSm
            AppButton {
                text: root.isLive ? Strings.get("btn_stop_stream")
                                  : Strings.get("btn_tune_in")
                iconName: root.isLive ? "pause" : "play"
                variant: root.isLive ? "danger" : "primary"
                onClicked: if (root.controller)
                    (root.isLive ? root.controller.stopStation()
                                 : root.controller.playCurrentStation())
            }
            AppButton {
                text: Strings.get("btn_reload")
                iconName: "refresh"; variant: "secondary"
                onClicked: if (root.controller) root.controller.reload()
            }
        }
    }
}
