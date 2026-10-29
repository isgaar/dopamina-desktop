/**
 * Location: src/features/library/ui/AudioQualityBadge.qml
 * Function: Audio fidelity badge (MAX, HIGH, OK, LOW) matching Android's
 * NowPlayingAudioBadge component and opening AudioQualityDialog on click.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    property var track: null
    readonly property string tier: {
        if (!root.track) return "MAX";
        var t = root.track.fidelityTier || root.track.formatBadge || "MAX";
        if (t === "MAX" || t === "HIGH" || t === "OK" || t === "LOW") return t;
        return "MAX";
    }
    readonly property color tierColor: {
        if (root.tier === "MAX") return Theme.primary;
        if (root.tier === "HIGH") return "#FFFFFF";
        if (root.tier === "OK") return "#4DB6AC";
        return "#8E8E93";
    }

    implicitHeight: 22
    implicitWidth: badgeLabel.implicitWidth + 16
    Layout.preferredHeight: 22
    Layout.preferredWidth: badgeLabel.implicitWidth + 16
    radius: 6
    color: badgeHover.containsMouse ?
        Qt.rgba(root.tierColor.r, root.tierColor.g, root.tierColor.b, 0.16) : "#14141C"
    border.width: 1
    border.color: badgeHover.containsMouse ?
        root.tierColor : Qt.rgba(root.tierColor.r, root.tierColor.g, root.tierColor.b, 0.40)

    Text {
        id: badgeLabel
        anchors.centerIn: parent
        text: root.tier
        font.family: Theme.fontFamily
        font.pixelSize: 10
        font.bold: true
        font.letterSpacing: 1.0
        color: root.tierColor
    }

    MouseArea {
        id: badgeHover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: qualityDialog.openForTrack(root.track)
    }

    AudioQualityDialog {
        id: qualityDialog
    }
}
