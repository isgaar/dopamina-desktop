/**
 * Location: src/features/stations/ui/delegates/TrackFormatBadge.qml
 * Function: Audio format chip displaying FLAC, MP3, or M4A codec tags with
 * distinct styling for lossless compression and audio quality classification.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../../shared/ui"

Rectangle {
    id: root
    property string audioFormat: ""
    readonly property bool isFlac: audioFormat === "FLAC"

    Layout.preferredWidth: 40; Layout.preferredHeight: 16
    radius: Theme.radiusSm; border.width: 1
    color: isFlac ? Theme.primaryDim : Theme.bgDark
    border.color: isFlac ? Theme.primary : Theme.borderDark

    Text {
        anchors.centerIn: parent; text: root.audioFormat
        color: root.isFlac ? Theme.primary : Theme.textDim
        font.pixelSize: 9; font.bold: true
        font.family: Theme.fontFamily
    }
}
