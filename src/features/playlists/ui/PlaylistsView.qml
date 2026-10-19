/**
 * Location: src/features/playlists/ui/PlaylistsView.qml
 * Function: Playlists view displaying user collections, creation triggers,
 * track collection item delegates, and empty state feedback when unpopulated.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"
import "../../../shared/ui/feedback"

Rectangle {
    id: root
    color: Theme.bgCard
    radius: Theme.radiusMd; clip: true
    border.color: Theme.borderDark; border.width: 1
    property var controller: null

    ColumnLayout {
        anchors.fill: parent; anchors.margins: Theme.spaceMd
        spacing: Theme.spaceSm

        RowLayout {
            Layout.fillWidth: true; spacing: Theme.spaceMd
            Text {
                text: Strings.get("nav_playlists")
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontLg; font.bold: true
                color: Theme.textLight
            }
            Item { Layout.fillWidth: true }
            Rectangle {
                width: 160; height: 32; color: Theme.bgSurface
                radius: Theme.radiusMd; border.color: Theme.borderDark
                border.width: 1
                TextInput {
                    id: newNameInput; anchors.fill: parent; anchors.margins: 4
                    color: Theme.textLight; font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSm; clip: true
                }
            }
            AppButton {
                text: Strings.get("btn_create")
                onClicked: {
                    var n = newNameInput.text.trim();
                    if (root.controller && n) {
                        root.controller.createPlaylist(n);
                        newNameInput.text = "";
                    }
                }
            }
        }

        ListView {
            id: playlistList
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            visible: count > 0
            model: root.controller ? root.controller.playlists : []
            delegate: Rectangle {
                width: playlistList.width; height: 48
                color: ma.containsMouse ? Theme.bgSurface : "transparent"
                radius: Theme.radiusSm
                RowLayout {
                    anchors.fill: parent; anchors.margins: Theme.spaceSm
                    spacing: Theme.spaceMd
                    AppIcon {
                        name: "list-music"; size: 20
                        iconColor: Theme.primaryHover
                    }
                    Text {
                        text: modelData.nombre || ""
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontMd; color: Theme.textLight
                        Layout.fillWidth: true; elide: Text.ElideRight
                    }
                    AppButton {
                        text: Strings.get("btn_delete")
                        onClicked: if (root.controller)
                            root.controller.deletePlaylist(modelData.id)
                    }
                }
                MouseArea { id: ma; anchors.fill: parent; hoverEnabled: true }
            }
        }

        EmptyStateView {
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: playlistList.count === 0
            iconName: "list-music"
            showAction: false
        }
    }
}
