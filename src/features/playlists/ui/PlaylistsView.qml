/**
 * Location: src/features/playlists/ui/PlaylistsView.qml
 * Function: Playlist collection with inline creation, deletion, track
 * counts, and empty-state feedback.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "../../../shared/ui"
import "../../../shared/ui/feedback"

Rectangle {
    id: root
    color: Theme.bgCard
    radius: Theme.radiusMd; clip: true
    border.color: Theme.borderDark; border.width: 1
    property var controller: null

    function requestCreate() {
        newNameInput.forceActiveFocus()
    }

    function createPlaylist() {
        var name = newNameInput.text.trim()
        if (root.controller && name) {
            root.controller.createPlaylist(name)
            newNameInput.text = ""
        }
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: Theme.spaceMd
        spacing: Theme.spaceSm

        RowLayout {
            Layout.fillWidth: true; spacing: Theme.spaceMd

            Text {
                text: Strings.get("nav_new_playlist")
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontMd; font.bold: true
                color: Theme.textLight
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 210; height: 34
                color: Theme.bgSurface
                radius: Theme.radiusMd
                border.color: Theme.borderDark; border.width: 1

                TextInput {
                    id: newNameInput
                    anchors.fill: parent; anchors.margins: 8
                    color: Theme.textLight
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSm
                    clip: true
                    onAccepted: root.createPlaylist()
                }
            }

            AppButton {
                text: Strings.get("btn_create")
                onClicked: root.createPlaylist()
            }
        }

        ListView {
            id: playlistList
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            visible: count > 0
            model: root.controller ? root.controller.playlists : []

            delegate: PlaylistRow {
                entry: modelData
                onDeleteRequested: if (root.controller)
                    root.controller.deletePlaylist(entry.id)
            }
        }

        EmptyStateView {
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: playlistList.count === 0
            iconName: "list-music"
            title: Strings.get("nav_playlists_hint")
            description: ""
            showAction: false
        }
    }

    Component.onCompleted: if (root.controller) root.controller.loadPlaylists()
}
