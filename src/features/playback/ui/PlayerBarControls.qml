/**
 * Location: src/features/playback/ui/PlayerBarControls.qml
 * Function: Central transport controls and a slim reactive duration bar.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "../../../shared/ui"
import "playerbar"

ColumnLayout {
    id: root
    Layout.fillWidth: true
    spacing: 3

    TransportControls {
        Layout.alignment: Qt.AlignHCenter
    }

    RowLayout {
        Layout.fillWidth: true
        Layout.maximumWidth: 520
        Layout.alignment: Qt.AlignHCenter
        spacing: 8

        Text {
            text: Theme.formatDuration(playbackController.position)
            font.family: Theme.fontFamily; font.pixelSize: 10
            color: Theme.textMuted
        }

        ProgressBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 3
            Layout.alignment: Qt.AlignVCenter
            from: 0
            to: Math.max(1, playbackController.duration)
            value: playbackController.position
            barColor: Theme.textLight
        }

        Text {
            text: Theme.formatDuration(playbackController.duration)
            font.family: Theme.fontFamily; font.pixelSize: 10
            color: Theme.textMuted
        }
    }
}
