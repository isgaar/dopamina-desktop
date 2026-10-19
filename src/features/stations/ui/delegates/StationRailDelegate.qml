/**
 * Location: src/features/stations/ui/delegates/StationRailDelegate.qml
 * Function: Station side rail delegate rendering frequency dials, station
 * name, genre categorization, and track counter badges with selection state.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../../shared/ui"

Rectangle {
    id: root
    width: ListView.view ? ListView.view.width : 260
    height: 46
    color: model.isActive ? Theme.bgSelected : (rM.containsMouse
        ? Theme.bgSurface : "transparent")

    property var controller: null

    Rectangle {
        width: 3; height: parent.height; color: Theme.primary
        visible: model.isActive
    }

    RowLayout {
        anchors.fill: parent; anchors.margins: Theme.spaceSm
        spacing: Theme.spaceSm

        ColumnLayout {
            Layout.fillWidth: true; spacing: 1
            Text {
                text: model.name; Layout.fillWidth: true
                color: model.isActive ? Theme.textLight : Theme.textMuted
                font.bold: model.isActive; font.pixelSize: Theme.fontSm
                font.family: Theme.fontFamily; elide: Text.ElideRight
            }
            Text {
                text: model.frequency + " • " + model.genre
                color: model.isActive ? Theme.primary : Theme.textDim
                font.pixelSize: 10; font.family: Theme.fontFamily
                elide: Text.ElideRight; Layout.fillWidth: true
            }
        }

        Rectangle {
            width: Math.max(22, cT.implicitWidth + 8); height: 18
            radius: Theme.radiusFull
            color: model.isActive ? Theme.primaryDim : Theme.bgDarkest
            border.color: model.isActive ? Theme.primary : Theme.borderDark
            border.width: 1
            Text {
                id: cT; anchors.centerIn: parent; text: model.trackCount
                color: model.isActive ? Theme.primary : Theme.textDim
                font.pixelSize: 10; font.bold: true
                font.family: Theme.fontFamily
            }
        }
    }

    MouseArea {
        id: rM; anchors.fill: parent; hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: if (root.controller)
            root.controller.selectStation(model.stationId)
        onDoubleClicked: if (root.controller)
            root.controller.playCurrentStation()
    }
}
