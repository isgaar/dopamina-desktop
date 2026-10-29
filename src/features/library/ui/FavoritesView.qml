/**
 * Location: src/features/library/ui/FavoritesView.qml
 * Function: Liked-track collection filtered from the loaded library and
 * refreshed whenever tracks or server likes change.
 * All Rights Reserved isgaar
 */
import QtQuick
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/feedback"

Rectangle {
    id: root
    color: Theme.bgCard
    radius: Theme.radiusMd; clip: true
    border.color: Theme.borderDark; border.width: 1

    property var libraryCtrl: null
    property var likesCtrl: null
    property var playbackCtrl: null
    property var queueCtrl: null
    property var tracks: []

    function refresh() {
        var source = root.libraryCtrl ? root.libraryCtrl.tracks : []
        var result = []
        for (var i = 0; i < source.length; ++i) {
            if (root.likesCtrl && root.likesCtrl.isLiked(source[i].id))
                result.push(source[i])
        }
        root.tracks = result
    }

    Connections {
        target: root.libraryCtrl
        function onTracksChanged() { root.refresh() }
    }

    Connections {
        target: root.likesCtrl
        function onLikesChanged() { root.refresh() }
    }

    Component.onCompleted: refresh()

    ListView {
        id: list
        anchors.fill: parent
        anchors.margins: Theme.spaceMd
        clip: true
        spacing: 8
        visible: count > 0
        model: root.tracks

        delegate: LibraryTrackDelegate {
            width: list.width
            currentPlayingId: root.playbackCtrl ?
                root.playbackCtrl.currentTrackId : 0
            currentPlayingTitle: root.playbackCtrl ?
                root.playbackCtrl.currentTrackTitle : ""
            onPlayRequested: t => root.playbackCtrl &&
                root.playbackCtrl.playLibraryTrack(
                    t.id || 0, t.titulo || t.title || "",
                    t.artista || t.artist || "",
                    t.album || "", t.path || "",
                    t.duracion || t.duration || 0,
                    t.coverUrl || t.cover_url || t.cover || "")
            onMixRequested: t => root.queueCtrl &&
                root.queueCtrl.startMix(t.id || 0, t.titulo || t.title || "")
        }
    }

    EmptyStateView {
        anchors.fill: parent
        anchors.margins: Theme.spaceMd
        visible: list.count === 0
        iconName: "heart"
        title: Strings.get("favorites_empty_title")
        description: Strings.get("favorites_empty_desc")
        showAction: false
    }
}
