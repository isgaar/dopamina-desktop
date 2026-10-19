/**
 * Location: src/features/stations/ui/delegates/TrackTableRowDelegate.qml
 * Function: Individual virtualized track table row rendering metadata, badges,
 * selection toggle checkboxes, and quick station reassignment triggers.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../../shared/ui"

Rectangle {
    id: root
    width: parent ? parent.width : 400; height: 38
    property var controller: null
    signal quickReassignRequested(int trackId)

    readonly property bool isInspected: root.controller &&
        root.controller.inspectedTrackId === model.trackId

    color: model.isSelected ? Theme.bgSelected : (isInspected
        ? Theme.bgCard : (rMouse.containsMouse
                          ? Theme.bgSurface : "transparent"))

    Rectangle {
        anchors.bottom: parent.bottom; width: parent.width; height: 1
        color: Theme.borderDark; opacity: 0.4
    }

    RowLayout {
        anchors.fill: parent; anchors.margins: 4; spacing: Theme.spaceSm

        Rectangle {
            width: 16; height: 16; radius: 3; border.width: 1
            color: model.isSelected ? Theme.primary : "transparent"
            border.color: model.isSelected ? Theme.primary
                                           : Theme.borderHighlight
            AppIcon {
                anchors.centerIn: parent; name: "check"; size: 10
                visible: model.isSelected; iconColor: Theme.bgDarkest
            }
            MouseArea {
                anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: if (root.controller)
                    root.controller.toggleTrackSelection(model.trackId)
            }
        }

        RowLayout {
            Layout.fillWidth: true; Layout.preferredWidth: 3; spacing: 4
            AppIcon {
                name: "music"; size: 12
                iconColor: isInspected ? Theme.primary : Theme.textDim
            }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 0
                Text {
                    text: model.title; Layout.fillWidth: true
                    color: isInspected ? Theme.primary : Theme.textLight
                    font.bold: true; font.pixelSize: 11
                    font.family: Theme.fontFamily; elide: Text.ElideRight
                }
                Text {
                    text: model.artist; Layout.fillWidth: true
                    color: Theme.textMuted; font.pixelSize: 10
                    font.family: Theme.fontFamily; elide: Text.ElideRight
                }
            }
        }

        Text {
            text: model.album !== "" ? model.album : "-"
            color: Theme.textDim; font.pixelSize: 10
            font.family: Theme.fontFamily; elide: Text.ElideRight
            Layout.fillWidth: true; Layout.preferredWidth: 2
        }

        Text {
            text: model.durationStr; color: Theme.textMuted
            font.pixelSize: 10; font.family: Theme.fontFamily
            Layout.preferredWidth: 48; horizontalAlignment: Text.AlignRight
        }

        TrackFormatBadge {
            audioFormat: model.audioFormat
        }

        TrackStationBadge {
            stationId: model.stationId
        }

        Rectangle {
            width: 22; height: 22; radius: Theme.radiusSm
            color: actM.containsMouse ? Theme.bgSurface : "transparent"
            AppIcon {
                anchors.centerIn: parent; name: "shuffle"; size: 11
                iconColor: actM.containsMouse ? Theme.primary : Theme.textDim
            }
            MouseArea {
                id: actM; anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: root.quickReassignRequested(model.trackId)
            }
        }
    }

    MouseArea {
        id: rMouse; anchors.fill: parent
        anchors.leftMargin: 24; anchors.rightMargin: 28
        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
        onClicked: if (root.controller)
            root.controller.inspectTrack(model.trackId)
    }
}
