/**
 * Location: src/features/playback/ui/PlayerBarControls.qml
 * Function: Central transport controls with play/pause, queue skip, and a
 * reactive duration progress bar slider.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"

ColumnLayout {
    id: root
    Layout.fillWidth: true; spacing: 4

    RowLayout {
        Layout.alignment: Qt.AlignHCenter; spacing: Theme.spaceSm
        Rectangle {
            width: 36; height: 36; radius: 18
            color: playMouse.containsMouse ?
                Theme.primaryHover : Theme.primary
            AppIcon {
                anchors.centerIn: parent; size: 16
                name: playbackController.isPlaying ? "pause" : "play"
                iconColor: Theme.bgDarkest
            }
            MouseArea {
                id: playMouse; anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: playbackController.togglePlay()
            }
        }
        Rectangle {
            readonly property bool hasQueue: queueController &&
                queueController.queueTracks.length > 0
            width: 30; height: 30; radius: 15
            opacity: hasQueue ? 1.0 : 0.4
            color: skipMouse.containsMouse && hasQueue ?
                Theme.bgSurface : "transparent"
            border.color: Theme.borderDark; border.width: 1
            AppIcon {
                anchors.centerIn: parent; name: "next"; size: 14
                iconColor: Theme.textLight
            }
            MouseArea {
                id: skipMouse; anchors.fill: parent
                enabled: parent.hasQueue
                hoverEnabled: true
                cursorShape: enabled ? Qt.PointingHandCursor :
                    Qt.ArrowCursor
                onClicked: queueController.next()
            }
        }
    }

    RowLayout {
        Layout.fillWidth: true; spacing: Theme.spaceSm
        Text {
            text: Theme.formatDuration(playbackController.position)
            font.family: Theme.fontFamily; font.pixelSize: 10
            color: Theme.textMuted
        }
        ProgressBar {
            Layout.fillWidth: true; from: 0
            to: Math.max(1, playbackController.duration)
            value: playbackController.position
        }
        Text {
            text: Theme.formatDuration(playbackController.duration)
            font.family: Theme.fontFamily; font.pixelSize: 10
            color: Theme.textMuted
        }
    }
}
