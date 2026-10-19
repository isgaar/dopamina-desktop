/**
 * Location: src/features/library/ui/LibraryView.qml
 * Function: Library view displaying searchable track list, album grid, and
 * artist collections with real-time discovery feedback and direct play.
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
    property var queueCtrl: null
    property var playbackCtrl: null
    property string section: "tracks"

    function sectionTitle() {
        if (section === "albums") return Strings.get("nav_albums");
        if (section === "artists") return Strings.get("nav_artists");
        if (section === "genres") return Strings.get("nav_genres");
        return section === "folders" ? Strings.get("nav_folders") :
                                       Strings.get("nav_tracks");
    }

    onSectionChanged: {
        if (!controller) return;
        section === "albums" ? controller.loadAlbums(1) :
            (section === "artists" ? controller.loadArtists(1) :
                                     controller.loadTracks(1));
    }

    ColumnLayout {
        anchors.fill: parent; anchors.margins: Theme.spaceMd
        spacing: Theme.spaceSm

        RowLayout {
            Layout.fillWidth: true; spacing: Theme.spaceMd

            Text {
                text: root.sectionTitle(); font.family: Theme.fontFamily
                font.pixelSize: Theme.fontLg; font.bold: true
                color: Theme.textLight
            }

            RowLayout {
                spacing: Theme.spaceSm
                visible: root.controller && root.controller.isLoading
                LoadingSpinner { size: 16; lineWidth: 2 }
                Text {
                    text: root.controller ? root.controller.statusMessage : ""
                    font.family: Theme.fontFamily; font.pixelSize: 11
                    color: Theme.accent
                }
            }

            Item { Layout.fillWidth: true }

            Rectangle {
                width: 240; height: 36; color: Theme.bgSurface
                radius: Theme.radiusMd; border.color: Theme.borderDark
                border.width: 1

                TextInput {
                    anchors.fill: parent; anchors.margins: Theme.spaceSm
                    color: Theme.textLight; font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontMd; clip: true
                    onTextChanged: if (root.controller)
                        root.controller.search(text)
                }
            }
        }

        ListView {
            id: trackList
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: count > 0
            model: root.controller ? (
                root.section === "albums" ? root.controller.albums :
                (root.section === "artists" ? root.controller.artists :
                 root.controller.tracks)) : []

            delegate: LibraryTrackDelegate {
                width: trackList.width
                onPlayRequested: t => root.playbackCtrl &&
                    root.playbackCtrl.playLibraryTrack(
                        t.id, t.titulo || t.title || "",
                        t.artista || t.artist || "",
                        t.album || "", t.path || "", t.duracion || 0)
                onRadioRequested: t => root.queueCtrl &&
                    root.queueCtrl.startRadio(t.id, t.titulo || t.title || "")
            }
        }

        EmptyStateView {
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: trackList.count === 0 &&
                (!root.controller || !root.controller.isLoading)
            iconName: root.section === "albums" ? "disc" :
                (root.section === "artists" ? "mic" : "music")
            showAction: true; actionText: Strings.get("btn_reload")
            onActionTriggered: if (root.controller)
                root.controller.loadTracks(1)
        }
    }

    Component.onCompleted: if (root.controller) root.controller.loadTracks(1)
}
