/**
 * Location: src/features/playback/ui/PlayerBar.qml
 * Function: Full-width persistent player bar with track identity, transport,
 * progress, queue shortcut, and volume control.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "../../../shared/ui"
import "playerbar"

Rectangle {
    id: root
    height: 74
    color: Theme.bgDark

    signal queueRequested()

    Rectangle {
        width: parent.width; height: 1
        color: Theme.borderDark
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceMd
        anchors.rightMargin: Theme.spaceMd
        anchors.topMargin: 7
        anchors.bottomMargin: 7
        spacing: Theme.spaceMd

        PlayerIdentity {
            Layout.preferredWidth: 360
            Layout.minimumWidth: 280
            Layout.maximumWidth: 400
            Layout.fillWidth: false
        }

        PlayerBarControls {
            Layout.fillWidth: true
            Layout.fillHeight: true
        }

        PlayerUtilities {
            Layout.preferredWidth: 240
            Layout.maximumWidth: 240
            Layout.fillWidth: false
            volume: sessionPlaybackController.isControllingAndroid ?
                sessionPlaybackController.remoteVolumePct :
                playbackController.volume
            onMoved: value => {
                if (sessionPlaybackController.isControllingAndroid)
                    sessionPlaybackController.remoteSetVolume(value);
                else
                    playbackController.setVolume(value);
            }
            onQueueRequested: root.queueRequested()
        }
    }
}
