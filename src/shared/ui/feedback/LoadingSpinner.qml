/**
 * Location: src/shared/ui/feedback/LoadingSpinner.qml
 * Function: Reusable fluid animated loading spinner ring adopted from Lumpy
 * Desktop with infinite RotationAnimator and DPI-aware stroke geometry.
 * All Rights Reserved isgaar
 */
import QtQuick

Item {
    id: root
    property int size: 36
    property int lineWidth: 3
    property color ringColor: Theme.accent
    property color dotColor: Theme.primaryHover
    property bool running: true

    implicitWidth: size
    implicitHeight: size

    Rectangle {
        id: ring
        anchors.fill: parent
        radius: width / 2
        color: "transparent"
        border.color: root.ringColor
        border.width: root.lineWidth
        opacity: root.running ? 1.0 : 0.0

        Behavior on opacity {
            NumberAnimation { duration: 200 }
        }

        Rectangle {
            width: Math.max(4, root.lineWidth * 2)
            height: width
            radius: width / 2
            color: root.dotColor
            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.topMargin: -root.lineWidth / 2
        }

        RotationAnimator {
            target: ring
            from: 0
            to: 360
            duration: 1000
            loops: Animation.Infinite
            running: root.running && root.visible
        }
    }
}
