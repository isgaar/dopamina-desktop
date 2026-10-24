/**
 * Location: src/shared/ui/RoundedMask.qml
 * Function: Invisible alpha mask exposing a rounded rectangle for MultiEffect
 * corner clipping without rendering any visible pixels.
 * All Rights Reserved isgaar
 */
import QtQuick
import "root:/src/shared"

Item {
    id: root
    property int cornerRadius: Theme.radiusMd

    visible: false
    layer.enabled: true

    Rectangle {
        anchors.fill: parent
        radius: root.cornerRadius
        color: "white"
    }
}
