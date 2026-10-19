/**
 * Location: src/features/stations/ui/StationManagerView.qml
 * Function: Master view integrating station side rail, audio profile banner,
 * search toolbar, virtualized track table, and station reassignment dialog.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"
import "dialogs"

Rectangle {
    id: root
    color: Theme.bgCard
    radius: root.isFloating ? Theme.radiusMd : 0
    border.color: Theme.borderDark
    border.width: 1
    clip: true

    property var controller: null
    property bool isFloating: true

    RowLayout {
        anchors.fill: parent
        spacing: 0

        StationSideRail {
            id: sideRail
            Layout.fillHeight: true
            controller: root.controller
            isFloating: root.isFloating
        }

        ColumnLayout {
            Layout.fillWidth: true; Layout.fillHeight: true
            spacing: 0

            StationHeaderBanner {
                id: headerBanner
                Layout.fillWidth: true
                controller: root.controller
            }

            TrackToolbar {
                id: toolbar
                Layout.fillWidth: true
                controller: root.controller
                onRequestReassign: {
                    reassignDialog.targetTrackId = 0
                    reassignDialog.open()
                }
            }

            TrackTableView {
                id: trackTable
                Layout.fillWidth: true; Layout.fillHeight: true
                controller: root.controller
                onQuickReassignRequested: function(tId) {
                    reassignDialog.targetTrackId = tId
                    reassignDialog.open()
                }
            }

            TrackDetailDrawer {
                id: detailDrawer
                Layout.fillWidth: true
                controller: root.controller
                onChangeStationRequested: function(tId) {
                    reassignDialog.targetTrackId = tId
                    reassignDialog.open()
                }
            }
        }
    }

    StationReassignDialog {
        id: reassignDialog
        anchors.centerIn: parent
        controller: root.controller
    }
}
