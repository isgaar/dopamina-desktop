/**
 * Location: src/features/stations/ui/dialogs/StationReassignDialog.qml
 * Function: Modal selection dialog for reassigning tracks to target FM stations
 * with atomic database persistence and user confirmation.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../../shared/ui"

Dialog {
    id: root
    width: 380; height: 320; modal: true
    title: Strings.get("dialog_reassign_title")

    property var controller: null
    property int targetTrackId: 0

    background: Rectangle {
        color: Theme.bgDark; border.color: Theme.primary
        border.width: 1; radius: Theme.radiusMd
    }

    contentItem: ColumnLayout {
        spacing: Theme.spaceMd

        Text {
            text: Strings.get("dialog_reassign_prompt")
            color: Theme.textLight; font.pixelSize: 12
            font.family: Theme.fontFamily; Layout.fillWidth: true
        }

        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true
            color: Theme.bgSurface; border.color: Theme.borderDark
            radius: Theme.radiusSm

            ListView {
                id: pList; anchors.fill: parent; anchors.margins: 4
                clip: true
                model: root.controller ? root.controller.stationsModel : null
                delegate: Rectangle {
                    width: pList.width; height: 30
                    color: pList.currentIndex === index
                        ? Theme.primaryDim
                        : (pM.containsMouse ? Theme.bgCard : "transparent")
                    radius: Theme.radiusSm

                    RowLayout {
                        anchors.fill: parent; anchors.margins: 6
                        spacing: Theme.spaceSm

                        AppIcon {
                            name: "radio"; size: 12
                            iconColor: pList.currentIndex === index
                                ? Theme.primary : Theme.textDim
                        }
                        Text {
                            text: model.name; Layout.fillWidth: true
                            color: pList.currentIndex === index
                                ? Theme.primary : Theme.textLight
                            font.pixelSize: 11; font.family: Theme.fontFamily
                            font.bold: pList.currentIndex === index
                            elide: Text.ElideRight
                        }
                        Text {
                            text: model.frequency; color: Theme.accentOrange
                            font.pixelSize: 10; font.family: Theme.fontFamily
                        }
                    }
                    MouseArea {
                        id: pM; anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: pList.currentIndex = index
                    }
                }
            }
        }

        RowLayout {
            Layout.alignment: Qt.AlignRight; spacing: Theme.spaceSm
            AppButton {
                text: Strings.get("btn_cancel"); variant: "secondary"
                onClicked: root.close()
            }
            AppButton {
                text: Strings.get("btn_apply")
                onClicked: {
                    if (!root.controller) return
                    var st = root.controller.stationsModel.getStation(
                        pList.currentIndex)
                    var targetId = (st && st.id) ? st.id :
                        root.controller.stationsModel.getStationId(
                            pList.currentIndex)
                    if (!targetId) return
                    if (root.targetTrackId > 0) {
                        root.controller.reassignTrack(
                            root.targetTrackId, targetId)
                    } else {
                        root.controller.reassignSelectedTracks(targetId)
                    }
                    root.close()
                }
            }
        }
    }
}
