/**
 * Location: src/features/library/ui/LibraryView.qml
 * Function: Library view displaying the alphabetical Hi-Fi track card list with
 * A-Z quick-jump index bar, album collection, and artist catalog.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "../../../shared/ui/feedback"

Item {
    id: root
    clip: true
    property var controller: null
    property var queueCtrl: null
    property var playbackCtrl: null
    property string section: "tracks"

    readonly property bool isTracksSection:
        root.section !== "albums" && root.section !== "artists"

    readonly property var alphabetLetters: [
        "#", "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L",
        "M", "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z"
    ]

    function jumpToLetter(letter) {
        if (!root.controller || !root.isTracksSection) return;
        var idx = root.controller.indexForSection(letter);
        if (idx >= 0 && idx < trackList.count) {
            trackList.positionViewAtIndex(idx, ListView.Beginning);
        }
    }

    onSectionChanged: {
        if (!controller) return;
        section === "albums" ? controller.loadAlbums(1) :
            (section === "artists" ? controller.loadArtists(1) :
                                     controller.loadTracks(1));
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: 6

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 8; Layout.rightMargin: 8
            Layout.topMargin: 4
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

        RowLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8
            clip: true
            visible: trackList.count > 0

            ListView {
                id: trackList
                Layout.fillWidth: true
                Layout.fillHeight: true
                clip: true
                spacing: 8
                boundsBehavior: Flickable.StopAtBounds
                model: root.controller ? (
                    root.section === "albums" ? root.controller.albums :
                    (root.section === "artists" ? root.controller.artists :
                     root.controller.tracks)) : []

                onAtYEndChanged: if (atYEnd && root.isTracksSection &&
                                     root.controller &&
                                     root.controller.hasMoreTracks)
                                     root.controller.loadMoreTracks()

                footer: Item {
                    width: trackList.width
                    height: root.isTracksSection && root.controller &&
                            root.controller.isLoading && trackList.count > 0 ?
                            48 : 12

                    LoadingSpinner {
                        anchors.centerIn: parent
                        size: 18; lineWidth: 2
                        visible: parent.height > 20
                    }
                }

                delegate: LibraryTrackDelegate {
                    width: trackList.width
                    isTracksSection: root.isTracksSection
                    currentPlayingId: root.playbackCtrl ?
                        root.playbackCtrl.currentTrackId : 0
                    currentPlayingTitle: root.playbackCtrl ?
                        root.playbackCtrl.currentTrackTitle : ""
                    onPlayRequested: t => {
                        if (root.queueCtrl && root.controller &&
                            root.isTracksSection) {
                            root.queueCtrl.playFromLibrary(
                                root.controller.tracks, index);
                        }
                        if (root.playbackCtrl) {
                            root.playbackCtrl.playLibraryTrack(
                                t.id || 0, t.titulo || t.title || t.name || "",
                                t.artista || t.artist || "",
                                t.album || "", t.path || "",
                                t.duracion || t.duration || 0,
                                t.coverUrl || t.cover_url || t.cover || "");
                        }
                    }
                    onMixRequested: t => root.queueCtrl &&
                        root.queueCtrl.startMix(
                            t.id || 0, t.titulo || t.title || "")
                }
            }

            Rectangle {
                id: alphabetBar
                visible: root.isTracksSection && trackList.count > 0
                Layout.preferredWidth: 24
                Layout.fillHeight: true
                Layout.topMargin: 2
                Layout.bottomMargin: 4
                radius: 12
                color: "#111116"
                border.color: "#23232D"
                border.width: 1

                property string activeLetter: ""

                ColumnLayout {
                    anchors.fill: parent
                    anchors.topMargin: 6
                    anchors.bottomMargin: 6
                    spacing: 0

                    Repeater {
                        model: root.alphabetLetters
                        delegate: Item {
                            Layout.fillWidth: true
                            Layout.fillHeight: true

                            Text {
                                anchors.centerIn: parent
                                text: modelData
                                font.family: Theme.fontFamily
                                font.pixelSize: 10
                                font.bold: true
                                color: alphabetBar.activeLetter === modelData ?
                                    Theme.accent : "#B0B0BE"
                            }
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    function pickAt(py) {
                        var usable = Math.max(1, height - 12);
                        var rel = Math.max(0, Math.min(1, (py - 6) / usable));
                        var idx = Math.min(
                            root.alphabetLetters.length - 1,
                            Math.floor(rel * root.alphabetLetters.length));
                        var letter = root.alphabetLetters[idx];
                        alphabetBar.activeLetter = letter;
                        root.jumpToLetter(letter);
                    }
                    onPressed: mouse => pickAt(mouse.y)
                    onPositionChanged: mouse => {
                        if (pressed) pickAt(mouse.y);
                    }
                    onReleased: alphabetBar.activeLetter = ""
                    onExited: if (!pressed) alphabetBar.activeLetter = ""
                }
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
