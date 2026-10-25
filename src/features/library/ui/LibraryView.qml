/**
 * Location: src/features/library/ui/LibraryView.qml
 * Function: Library view displaying the searchable track list, album grid,
 * and artist collections with live loading feedback and direct playback.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "../../../shared/ui/feedback"

Item {
    id: root
    property var controller: null
    property var queueCtrl: null
    property var playbackCtrl: null
    property string section: "tracks"

    onSectionChanged: {
        if (!controller) return;
        section === "albums" ? controller.loadAlbums(1) :
            (section === "artists" ? controller.loadArtists(1) :
                                     controller.loadTracks(1));
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 0

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 12; Layout.rightMargin: 12
            Layout.topMargin: 8
            spacing: Theme.spaceSm
            visible: root.controller && root.controller.isLoading

            LoadingSpinner { size: 16; lineWidth: 2 }
            Text {
                text: root.controller ? root.controller.statusMessage : ""
                font.family: Theme.fontFamily
                font.pixelSize: Theme.fontSm
                color: Theme.accent
            }
            Item { Layout.fillWidth: true }
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
                onMixRequested: t => root.queueCtrl &&
                    root.queueCtrl.startMix(t.id, t.titulo || t.title || "")
            }
        }

        LibraryEmptyState {
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: trackList.count === 0 &&
                (!root.controller || !root.controller.isLoading)
            section: root.section
        }
    }

    Component.onCompleted: if (root.controller) root.controller.loadTracks(1)
}
