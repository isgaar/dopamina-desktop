/**
 * Location: src/features/stations/ui/delegates/TrackStationBadge.qml
 * Function: Station badge component indicating assigned radio station or
 * unassigned status with dynamic theme accents and rounded border badges.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../../shared/ui"

Rectangle {
    id: root
    property string stationId: ""
    readonly property bool hasStation: stationId !== ""

    Layout.preferredWidth: 95; Layout.preferredHeight: 18
    radius: Theme.radiusSm; border.width: 1
    color: hasStation ? Theme.bgSurface : Theme.bgDark
    border.color: hasStation ? Theme.accent : Theme.warning

    Text {
        anchors.centerIn: parent; elide: Text.ElideRight
        text: root.hasStation ? root.stationId : Strings.get("rail_unassigned")
        color: root.hasStation ? Theme.accent : Theme.warning
        font.pixelSize: 10; font.bold: true
        font.family: Theme.fontFamily
    }
}
