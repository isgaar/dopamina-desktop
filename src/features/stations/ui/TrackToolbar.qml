/**
 * Location: src/features/stations/ui/TrackToolbar.qml
 * Function: Action toolbar hosting real-time metadata search, audio format
 * filter chips, and batch station reassignment trigger controls.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"

Rectangle {
    id: root
    height: 44; color: Theme.bgDark
    border.color: Theme.borderDark; border.width: 1

    property var controller: null
    signal requestReassign()

    RowLayout {
        anchors.fill: parent; anchors.margins: Theme.spaceSm
        spacing: Theme.spaceMd

        Rectangle {
            Layout.preferredWidth: 300; Layout.preferredHeight: 30
            radius: Theme.radiusSm; color: Theme.bgSurface
            border.color: sIn.activeFocus ? Theme.primary : Theme.borderDark
            border.width: 1

            RowLayout {
                anchors.fill: parent; anchors.margins: 4; spacing: 4
                AppIcon {
                    name: "search"; size: 12
                    iconColor: sIn.activeFocus ? Theme.primary : Theme.textDim
                }
                TextInput {
                    id: sIn; Layout.fillWidth: true; color: Theme.textLight
                    font.pixelSize: 11; font.family: Theme.fontFamily
                    selectByMouse: true
                    onTextChanged: if (root.controller)
                        root.controller.search(text)
                    Text {
                        anchors.fill: parent
                        text: Strings.get("search_placeholder")
                        color: Theme.textDim; font.pixelSize: 11
                        font.family: Theme.fontFamily
                        visible: !sIn.text && !sIn.activeFocus
                    }
                }
            }
        }

        RowLayout {
            spacing: 4
            Repeater {
                model: [
                    { id: "all", label: Strings.get("filter_all") },
                    { id: "mp3", label: "MP3" },
                    { id: "flac", label: "FLAC" },
                    { id: "m4a", label: "M4A" }
                ]
                delegate: Rectangle {
                    readonly property bool sel: root.controller &&
                        root.controller.formatFilter === modelData.id
                    width: cText.implicitWidth + 12; height: 24
                    radius: Theme.radiusSm
                    color: sel ? Theme.primaryDim : Theme.bgSurface
                    border.color: sel ? Theme.primary : Theme.borderDark
                    border.width: 1
                    Text {
                        id: cText; anchors.centerIn: parent
                        text: modelData.label
                        color: sel ? Theme.primary : Theme.textMuted
                        font.pixelSize: 10; font.family: Theme.fontFamily
                        font.bold: sel
                    }
                    MouseArea {
                        anchors.fill: parent; hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: if (root.controller)
                            root.controller.setFormatFilter(modelData.id)
                    }
                }
            }
        }

        Item { Layout.fillWidth: true }

        RowLayout {
            spacing: Theme.spaceSm
            Text {
                visible: root.controller && root.controller.tracksModel &&
                         root.controller.tracksModel.selectedCount > 0
                text: (root.controller ?
                       root.controller.tracksModel.selectedCount : 0) +
                      " seleccionadas"
                color: Theme.accent; font.pixelSize: 11
                font.bold: true; font.family: Theme.fontFamily
            }
            AppButton {
                text: Strings.get("btn_reassign")
                iconName: "shuffle"; variant: "secondary"
                enabled: root.controller && root.controller.tracksModel &&
                         root.controller.tracksModel.selectedCount > 0
                onClicked: root.requestReassign()
            }
        }
    }
}
