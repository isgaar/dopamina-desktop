/**
 * Location: src/shared/ui/window/SleepTimerDialog.qml
 * Function: Sleep timer modal window allowing users to set timed playback
 * cessation with Apple fluid motion physics and countdown readouts.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."
import "../.."

RoundedWindow {
    id: root
    width: 360; height: 260
    minimumWidth: 320; minimumHeight: 220
    maximumWidth: 420; maximumHeight: 300
    title: Strings.get("timer_title")
    headerTitle: title

    property int minutes: 30
    property int remainingSecs: 0
    property bool timerActive: false

    Timer {
        id: countdownTimer
        interval: 1000; repeat: true; running: root.timerActive
        onTriggered: {
            if (root.remainingSecs > 1) {
                root.remainingSecs -= 1
            } else {
                root.timerActive = false
                if (playbackController) playbackController.pause()
                root.close()
            }
        }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: Theme.spaceMd
        spacing: Theme.spaceSm

        Text {
            text: root.timerActive ?
                Strings.get("timer_stop_after") + " " +
                Math.floor(root.remainingSecs / 60) + "m " +
                (root.remainingSecs % 60) + "s" :
                Strings.get("timer_stop_after") + " " + root.minutes + " min"
            font.family: Theme.fontFamily; font.pixelSize: 13
            font.bold: true; color: Theme.textLight
            Layout.alignment: Qt.AlignHCenter
        }

        Slider {
            Layout.fillWidth: true; enabled: !root.timerActive
            from: 5; to: 120; stepSize: 5; value: root.minutes
            onMoved: root.minutes = Math.round(value)
        }

        RowLayout {
            Layout.fillWidth: true; spacing: Theme.spaceXs
            Repeater {
                model: [15, 30, 45, 60]
                delegate: AppButton {
                    Layout.fillWidth: true; Layout.preferredHeight: 28
                    text: modelData + "m"; variant: "secondary"
                    enabled: !root.timerActive
                    onClicked: root.minutes = modelData
                }
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true; spacing: Theme.spaceSm
            AppButton {
                Layout.fillWidth: true
                text: root.timerActive ?
                    Strings.get("timer_cancel") : Strings.get("timer_start")
                variant: root.timerActive ? "danger" : "primary"
                onClicked: {
                    if (root.timerActive) {
                        root.timerActive = false
                    } else {
                        root.remainingSecs = root.minutes * 60
                        root.timerActive = true
                    }
                }
            }
        }
    }
}
