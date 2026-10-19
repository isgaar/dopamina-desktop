/**
 * Location: src/features/stations/ui/TrackTableView.qml
 * Function: High-performance track table view displaying audio metadata, format
 * badges, selection checkboxes, and quick station reassignment triggers.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"
import "delegates"

Rectangle {
    id: root
    color: "transparent"

    property var controller: null
    signal quickReassignRequested(int trackId)

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 30
            color: Theme.bgDark; border.color: Theme.borderDark; border.width: 1

            RowLayout {
                anchors.fill: parent; anchors.margins: Theme.spaceSm
                spacing: Theme.spaceSm

                Item {
                    width: 20; height: 20
                    AppIcon {
                        anchors.centerIn: parent; name: "check"
                        size: 12; iconColor: Theme.textDim
                    }
                    MouseArea {
                        anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            if (!root.controller ||
                                !root.controller.tracksModel) return
                            var m = root.controller.tracksModel
                            root.controller.selectAllTracks(
                                m.selectedCount === 0)
                        }
                    }
                }

                Text {
                    text: Strings.get("table_col_title"); color: Theme.textMuted
                    font.bold: true; font.pixelSize: 11
                    font.family: Theme.fontFamily; Layout.fillWidth: true
                    Layout.preferredWidth: 3
                }

                Text {
                    text: Strings.get("table_col_album"); color: Theme.textMuted
                    font.bold: true; font.pixelSize: 11
                    font.family: Theme.fontFamily; Layout.fillWidth: true
                    Layout.preferredWidth: 2
                }

                Text {
                    text: Strings.get("table_col_duration")
                    color: Theme.textMuted; font.bold: true; font.pixelSize: 11
                    font.family: Theme.fontFamily; Layout.preferredWidth: 54
                    horizontalAlignment: Text.AlignRight
                }

                Text {
                    text: Strings.get("table_col_format")
                    color: Theme.textMuted; font.bold: true; font.pixelSize: 11
                    font.family: Theme.fontFamily; Layout.preferredWidth: 44
                    horizontalAlignment: Text.AlignHCenter
                }

                Text {
                    text: Strings.get("table_col_station")
                    color: Theme.textMuted; font.bold: true; font.pixelSize: 11
                    font.family: Theme.fontFamily; Layout.preferredWidth: 105
                }

                Item { width: 24 }
            }
        }

        ListView {
            id: trackListView
            Layout.fillWidth: true; Layout.fillHeight: true
            clip: true; boundsBehavior: Flickable.StopAtBounds
            model: root.controller ? root.controller.tracksModel : null
            ScrollBar.vertical: ScrollBar {
                active: true; policy: ScrollBar.AsNeeded
            }
            delegate: TrackTableRowDelegate {
                width: trackListView.width
                controller: root.controller
                onQuickReassignRequested: function(tId) {
                    root.quickReassignRequested(tId)
                }
            }
        }
    }
}
