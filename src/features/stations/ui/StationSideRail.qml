/**
 * Location: src/features/stations/ui/StationSideRail.qml
 * Function: Left navigation side rail displaying 12 FM stations with frequency
 * dials, unassigned filters, and track count badges for music curation.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"
import "delegates"

Item {
    id: root
    width: 260
    property var controller: null
    property bool isFloating: true

    Rectangle {
        anchors.fill: parent
        color: Theme.bgDark
        radius: root.isFloating ? Theme.radiusMd : 0
    }

    Rectangle {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        width: 1
        color: Theme.borderDark
    }

    ColumnLayout {
        anchors.fill: parent; spacing: 0

        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 44
            color: "transparent"
            RowLayout {
                anchors.fill: parent; anchors.margins: Theme.spaceSm
                spacing: Theme.spaceSm
                AppIcon {
                    name: "radio-tower"; size: 16; iconColor: Theme.primary
                }
                Text {
                    text: Strings.get("rail_stations_title")
                    color: Theme.textLight; font.bold: true
                    font.pixelSize: Theme.fontSm
                    font.family: Theme.fontFamily; Layout.fillWidth: true
                }
            }
            Rectangle {
                anchors.bottom: parent.bottom; width: parent.width; height: 1
                color: Theme.borderDark
            }
        }

        Repeater {
            model: [
                { id: "all", name: Strings.get("rail_all_stations"),
                  icon: "disc", col: Theme.primary },
                { id: "unassigned", name: Strings.get("rail_unassigned"),
                  icon: "shuffle", col: Theme.warning }
            ]
            delegate: Rectangle {
                readonly property bool active: root.controller &&
                    root.controller.currentStationId === modelData.id
                Layout.fillWidth: true; Layout.preferredHeight: 36
                color: active ? Theme.bgSelected : (fM.containsMouse
                    ? Theme.bgSurface : "transparent")
                RowLayout {
                    anchors.fill: parent; anchors.margins: Theme.spaceSm
                    spacing: Theme.spaceSm
                    AppIcon {
                        name: modelData.icon; size: 14
                        iconColor: active ? modelData.col : Theme.textDim
                    }
                    Text {
                        text: modelData.name; Layout.fillWidth: true
                        color: active ? Theme.textLight : Theme.textMuted
                        font.bold: active; font.pixelSize: Theme.fontSm
                        font.family: Theme.fontFamily
                    }
                }
                MouseArea {
                    id: fM; anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: if (root.controller)
                        root.controller.selectStation(modelData.id)
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 1
            color: Theme.borderDark
        }

        ListView {
            id: sList; Layout.fillWidth: true; Layout.fillHeight: true
            clip: true
            model: root.controller ? root.controller.stationsModel : null
            boundsBehavior: Flickable.StopAtBounds
            delegate: StationRailDelegate {
                controller: root.controller
            }
        }
    }
}
