/**
 * Location: src/shared/ui/RoundedImage.qml
 * Function: Image wrapper with true four-corner clipping through a MultiEffect
 * alpha mask, reusable for artwork thumbnails across the desktop client.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Effects
import "root:/src/shared"

Item {
    id: root
    property alias source: image.source
    property int radius: Theme.radiusSm
    property int fillMode: Image.PreserveAspectCrop
    readonly property bool hasImage:
        image.status === Image.Ready && image.source.toString().length > 0

    Image {
        id: image
        anchors.fill: parent
        fillMode: root.fillMode
        visible: false
    }

    MultiEffect {
        source: image
        anchors.fill: parent
        visible: root.hasImage
        maskEnabled: true
        maskSource: mask
    }

    Item {
        id: mask
        anchors.fill: parent
        visible: false
        layer.enabled: true
        Rectangle {
            anchors.fill: parent
            radius: root.radius
            color: "white"
        }
    }
}
