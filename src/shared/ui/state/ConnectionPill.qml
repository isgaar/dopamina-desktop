/**
 * Location: src/shared/ui/state/ConnectionPill.qml
 * Function: Status pill displaying connection service level (Full, SMB-only,
 * or Offline), transport information, and descriptive diagnostics tooltip.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import "root:/src/shared"
import "root:/src/shared/ui"

Rectangle {
    id: root

    property int level: connectionService ? connectionService.level : 2
    property string transport: "None"
    property string statusText: connectionService
        ? connectionService.statusMessage : Strings.get("state_offline_title")

    implicitHeight: 24
    implicitWidth: contentRow.implicitWidth + Theme.spaceMd * 2
    radius: Theme.radiusPill
    color: Theme.bgSurface
    border.color: Theme.borderDark
    border.width: 1

    function pillColor() {
        if (root.level === 0) return Theme.statusConnected;
        if (root.level === 1) return Theme.statusWarning;
        return Theme.statusDisconnected;
    }

    Row {
        id: contentRow
        anchors.centerIn: parent
        spacing: Theme.spaceSm

        Rectangle {
            width: 7; height: 7; radius: 3.5
            anchors.verticalCenter: parent.verticalCenter
            color: root.pillColor()
        }

        Text {
            anchors.verticalCenter: parent.verticalCenter
            text: connectionService
                ? connectionService.levelString
                : Strings.get("state_offline_title")
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontXs

            font.bold: true
            color: Theme.textLight
        }
    }

    ToolTip.visible: mouseArea.containsMouse
    ToolTip.text: root.statusText

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            if (connectionService) connectionService.checkConnection();
        }
    }
}
