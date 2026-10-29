/**
 * Location: src/features/playback/ui/playerbar/TransportControls.qml
 * Function: Shuffle, previous, play/pause, next, and repeat transport row
 * with a white primary action matching the Android reference.
 * All Rights Reserved isgaar
 */
import QtQuick
import "../../../../shared/ui"

Row {
    id: root
    spacing: 8

    TransportButton {
        anchors.verticalCenter: parent.verticalCenter
        iconName: "shuffle"; iconSize: 15
        active: queueController.isShuffle
        onClicked: queueController.toggleShuffle()
    }
    TransportButton {
        anchors.verticalCenter: parent.verticalCenter
        iconName: "skip-back"; iconSize: 16
        onClicked: queueController.previous()
    }

    Rectangle {
        id: play
        anchors.verticalCenter: parent.verticalCenter
        width: 42; height: 42; radius: 21
        color: playHover.containsMouse ?
            Theme.withAlpha(Theme.textLight, 0.88) : Theme.textLight

        Rectangle {
            anchors.centerIn: parent
            width: 52; height: 52; radius: 26
            color: Theme.withAlpha(Theme.textLight, 0.06)
        }

        AppIcon {
            anchors.centerIn: parent
            size: 17
            name: (playbackController.isPlaying ||
                   sessionPlaybackController.remotePlaying) ? "pause" : "play"
            iconColor: Theme.bgDarkest
        }

        MouseArea {
            id: playHover
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (sessionPlaybackController.isControllingAndroid)
                    sessionPlaybackController.toggleRemotePlay();
                else
                    playbackController.togglePlay();
            }
        }
    }

    TransportButton {
        anchors.verticalCenter: parent.verticalCenter
        iconName: "skip-forward"; iconSize: 16
        onClicked: queueController.next()
    }
    TransportButton {
        anchors.verticalCenter: parent.verticalCenter
        iconName: "repeat"; iconSize: 16
        active: queueController.isRepeat
        onClicked: queueController.toggleRepeat()
    }
}
