/**
 * Location: src/features/playback/ui/playerbar/PlayerSessionSource.qml
 * Function: Interactive badge identifying the active playback device (Desktop,
 * Android, or Web) and allowing one-click switching between Android and Desktop.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "../../../../shared/ui"

RowLayout {
    id: root
    spacing: 6
    visible: sessionPlaybackController.hasSession ||
             playbackController.currentTrackTitle.length > 0

    readonly property bool isAndroidActive:
        sessionPlaybackController.activeDeviceKind === "mobile"

    AppIcon {
        name: root.isAndroidActive ? "smartphone" : "monitor"
        size: 11
        iconColor: Theme.primary
        Layout.alignment: Qt.AlignVCenter
    }

    Text {
        Layout.fillWidth: false
        Layout.maximumWidth: 185
        text: {
            var kind = sessionPlaybackController.activeDeviceKind;
            return kind === "mobile" ?
                Strings.get("session_source_mobile") :
                (kind === "web" ? Strings.get("session_source_web") :
                 Strings.get("session_source_desktop"));
        }
        font.family: Theme.fontFamily
        font.pixelSize: 11
        color: Theme.primary
        elide: Text.ElideRight
    }

    Rectangle {
        visible: sessionPlaybackController.canSwitchToDesktop ||
                 sessionPlaybackController.canSwitchToAndroid
        Layout.preferredHeight: 18
        Layout.preferredWidth: switchLabel.implicitWidth + 14
        radius: 9
        color: switchHover.containsMouse ? Theme.primary :
            Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.12)
        border.color: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.45)
        border.width: 1

        Text {
            id: switchLabel
            anchors.centerIn: parent
            text: sessionPlaybackController.canSwitchToDesktop ?
                Strings.get("session_switch_to_desktop") :
                Strings.get("session_switch_to_android")
            font.family: Theme.fontFamily
            font.pixelSize: 10
            font.bold: true
            color: switchHover.containsMouse ? "#FFFFFF" : Theme.primary
        }

        MouseArea {
            id: switchHover
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (sessionPlaybackController.canSwitchToDesktop)
                    sessionPlaybackController.switchToDesktop();
                else
                    sessionPlaybackController.switchToAndroid();
            }
        }
    }

    Item { Layout.fillWidth: true }
}
