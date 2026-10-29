/**
 * Location: src/shared/ui/feedback/ProgressBar.qml
 * Function: Reusable fluid animated progress bar supporting determinate and
 * indeterminate states with design token color themes and rounded geometries.
 * All Rights Reserved isgaar
 */
import QtQuick

Rectangle {
    id: root
    property real from: 0.0
    property real to: 1.0
    property real value: 0.0
    property bool indeterminate: false
    property color barColor: Theme.accent
    signal moved(real newValue)
    readonly property real progress: to > from ?
        Math.max(0.0, Math.min(1.0, (value - from) / (to - from))) : 0.0

    implicitWidth: 200
    implicitHeight: 4
    radius: height / 2
    color: Theme.bgSurface
    clip: true

    Rectangle {
        id: fillBar
        height: parent.height
        radius: parent.radius
        color: root.barColor
        width: root.indeterminate ? parent.width * 0.35 :
            Math.round(parent.width * root.progress)

        SequentialAnimation on x {
            running: root.indeterminate && root.visible
            loops: Animation.Infinite
            NumberAnimation {
                from: -fillBar.width
                to: root.width
                duration: 1200
                easing.type: Easing.InOutQuad
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        anchors.topMargin: -6
        anchors.bottomMargin: -6
        cursorShape: Qt.PointingHandCursor
        onClicked: mouse => {
            if (root.width <= 0 || root.to <= root.from) return;
            var ratio = Math.max(0.0, Math.min(1.0, mouse.x / root.width));
            root.moved(root.from + ratio * (root.to - root.from));
        }
    }
}
