/**
 * Location: src/features/library/ui/AddToPlaylistDialog.qml
 * Function: Modal dialog allowing the user to add a track to an existing
 * playlist or create a new playlist on the fly.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"

Popup {
    id: root
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    width: 380
    padding: 20
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var track: null

    function openForTrack(t) {
        root.track = t;
        newPlaylistInput.text = "";
        if (typeof playlistController !== "undefined" && playlistController) {
            playlistController.loadPlaylists();
        }
        root.open();
    }

    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.65) }

    background: Rectangle {
        radius: 18
        color: "#15151E"
        border.color: "#2C2C3C"
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 12

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: Strings.get("pl_picker_title") || "Agregar a playlist"
                font.family: Theme.fontFamily
                font.pixelSize: 16
                font.bold: true
                color: "#F5F5FA"
                Layout.fillWidth: true
            }
            AppIcon {
                name: "close"; size: 16
                iconColor: closePlArea.containsMouse ? "#FFFFFF" : "#9494A6"
                MouseArea {
                    id: closePlArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: 8

            Rectangle {
                Layout.fillWidth: true
                Layout.preferredHeight: 34
                radius: 8
                color: "#1E1E2A"
                border.color: newPlaylistInput.activeFocus ? Theme.accent : "#2E2E3E"
                border.width: 1

                TextInput {
                    id: newPlaylistInput
                    anchors.fill: parent
                    anchors.margins: 8
                    color: "#F5F5FA"
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                    clip: true
                    onAccepted: createBtn.clicked()

                    Text {
                        anchors.fill: parent
                        text: Strings.get("new_playlist_name_hint") || "Nombre de nueva playlist..."
                        color: "#6E6E80"
                        font: parent.font
                        visible: parent.text.length === 0 && !parent.activeFocus
                    }
                }
            }

            AppButton {
                id: createBtn
                text: Strings.get("btn_create") || "Crear"
                variant: "primary"
                onClicked: {
                    var name = newPlaylistInput.text.trim();
                    if (name.length > 0 && typeof playlistController !== "undefined" &&
                        playlistController) {
                        playlistController.createPlaylist(name);
                        newPlaylistInput.text = "";
                    }
                }
            }
        }

        ListView {
            id: plList
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(220, Math.max(60, contentHeight))
            clip: true
            spacing: 6
            model: (typeof playlistController !== "undefined" && playlistController) ?
                   playlistController.playlists : []

            delegate: Rectangle {
                width: plList.width
                height: 42
                radius: 10
                color: plHover.containsMouse ? "#252534" : "#1C1C26"
                border.color: plHover.containsMouse ? Theme.accent : "#2B2B3A"
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.leftMargin: 12
                    anchors.rightMargin: 12
                    spacing: 10

                    AppIcon {
                        name: "list-music"; size: 15
                        iconColor: Theme.accent
                    }
                    Text {
                        text: modelData.nombre || modelData.name || "Playlist"
                        font.family: Theme.fontFamily
                        font.pixelSize: 13
                        font.bold: true
                        color: "#F5F5FA"
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }
                    AppIcon {
                        name: "plus"; size: 14
                        iconColor: "#B0B0C0"
                    }
                }

                MouseArea {
                    id: plHover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        var tid = root.track ? (root.track.id || 0) : 0;
                        var pid = modelData.id || 0;
                        if (typeof playlistController !== "undefined" &&
                            playlistController && pid > 0 && tid > 0) {
                            playlistController.addTrackToPlaylist(pid, tid);
                        }
                        root.close();
                    }
                }
            }
        }
    }
}
