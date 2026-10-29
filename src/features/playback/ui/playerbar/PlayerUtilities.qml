/**
 * Location: src/features/playback/ui/playerbar/PlayerUtilities.qml
 * Function: Queue shortcut, volume glyph, and slim custom volume slider for
 * the persistent player bar.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
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

    AppIcon {
        Layout.alignment: Qt.AlignVCenter
        name: "volume-2"; size: 17
        iconColor: Theme.textMuted
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
