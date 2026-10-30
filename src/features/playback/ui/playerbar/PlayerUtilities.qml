/**
 * Location: src/features/playback/ui/playerbar/PlayerUtilities.qml
 * Function: Queue shortcut, volume glyph, and slim custom volume slider for
 * the persistent player bar.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "../../../../shared/ui"

RowLayout {
    id: root
    property int volume: 0
    signal moved(int value)
    signal queueRequested()

    spacing: 8

    TransportButton {
        Layout.alignment: Qt.AlignVCenter
        iconName: "list-music"; iconSize: 17
        onClicked: root.queueRequested()
    }

    readonly property bool isMobileActive:
        (typeof sessionPlaybackController !== "undefined" && sessionPlaybackController) ?
        sessionPlaybackController.isControllingAndroid : false

    Rectangle {
        id: deviceVolumeBtn
        Layout.alignment: Qt.AlignVCenter
        Layout.preferredWidth: 26
        Layout.preferredHeight: 26
        radius: 13
        color: deviceHover.containsMouse ?
            (root.isMobileActive ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.20) :
                                   Qt.rgba(Theme.textLight.r, Theme.textLight.g, Theme.textLight.b, 0.08)) :
            (root.isMobileActive ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.12) :
                                   "transparent")
        border.color: root.isMobileActive ? Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.40) : "transparent"
        border.width: 1

        ToolTip.visible: deviceHover.containsMouse
        ToolTip.delay: 350
        ToolTip.text: root.isMobileActive ?
            qsTr("Sesión móvil en Android (Clic para pasar a este equipo)") :
            qsTr("Sesión en este equipo (Clic para pasar a Android)")

        AppIcon {
            anchors.centerIn: parent
            name: {
                if (root.isMobileActive) return "smartphone";
                if (root.volume === 0) return "volume-x";
                if (root.volume < 50) return "volume-1";
                return "volume-2";
            }
            size: root.isMobileActive ? 15 : 17
            iconColor: root.isMobileActive ? Theme.primary :
                (deviceHover.containsMouse ? Theme.textLight : Theme.textMuted)
        }

        MouseArea {
            id: deviceHover
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                if (typeof sessionPlaybackController === "undefined" || !sessionPlaybackController)
                    return;
                if (root.isMobileActive) {
                    sessionPlaybackController.switchToDesktop();
                } else {
                    sessionPlaybackController.switchToAndroid();
                }
            }
        }
    }

    Item {
        id: slider
        Layout.fillWidth: true
        Layout.preferredHeight: 18

        readonly property real ratio:
            Math.max(0, Math.min(1, root.volume / 100))

        function seek(px) {
            var usable = Math.max(1, width - 10);
            var r = Math.max(0, Math.min(1, (px - 5) / usable));
            root.moved(Math.round(r * 100));
        }

        Rectangle {
            id: track
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width; height: 4; radius: 2
            color: Theme.bgSurface
        }

        Rectangle {
            id: fill
            anchors.verticalCenter: parent.verticalCenter
            width: 2 + (parent.width - 4) * parent.ratio
            height: 4; radius: 2
            color: Theme.primary
        }

        Rectangle {
            width: 10; height: 10; radius: 5
            color: Theme.primary
            anchors.verticalCenter: parent.verticalCenter
            x: Math.max(-1, Math.min(parent.width - width + 1,
                                     fill.width - width / 2))
        }

        MouseArea {
            anchors.fill: parent
            onPressed: slider.seek(mouse.x)
            onPositionChanged: if (pressed) slider.seek(mouse.x)
        }
    }
}
