/**
 * Location: src/shared/ui/navigation/MainDashboard.qml
 * Function: Primary music workspace integrating the navigation rail, resizer
 * splitter, section tabs, dynamic content views, and persistent player bar.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui/navigation"
import "root:/src/features/playback/ui"
import "root:/src/features/library/ui"
import "root:/src/features/playlists/ui"
import "root:/src/features/queue/ui"

Item {
    id: root
    property string activeView: "tracks"
    signal openSettingsRequested()
    signal openEqualizerRequested()
    signal openTimerRequested()
    signal openAboutRequested()

    PlayerBar {
        id: playerBar
        anchors.left: parent.left; anchors.right: parent.right
        anchors.bottom: parent.bottom
        onQueueRequested: root.activeView = "queue"
    }

    SideNav {
        id: sideNav
        anchors.top: parent.top; anchors.left: parent.left
        anchors.bottom: playerBar.top
        currentMode: root.activeView
        onModeSelected: m => root.activeView = m
        onNewPlaylistRequested: {
            root.activeView = "playlists"; playlistsView.requestCreate()
        }
        onOpenSettingsRequested: root.openSettingsRequested()
        onOpenEqualizerRequested: root.openEqualizerRequested()
        onOpenTimerRequested: root.openTimerRequested()
        onOpenAboutRequested: root.openAboutRequested()
    }

    SidebarResizer {
        x: sideNav.width - 3; sideNav: sideNav
        anchors.top: sideNav.top; anchors.bottom: sideNav.bottom
        settingsController: windowRoot.settingsController
        maxContainerWidth: root.width
    }

    ColumnLayout {
        id: content
        anchors.top: parent.top; anchors.left: sideNav.right
        anchors.right: parent.right; anchors.bottom: playerBar.top
        anchors.margins: Theme.spaceMd; anchors.bottomMargin: 0
        spacing: Theme.spaceMd

        ContentTabs {
            Layout.fillWidth: true; current: root.activeView
            onSelected: mode => root.activeView = mode
        }

        Item {
            Layout.fillWidth: true; Layout.fillHeight: true

            LibraryView {
                id: libraryView; anchors.fill: parent
                visible: ["tracks", "albums", "artists", "genres",
                          "folders"].indexOf(root.activeView) !== -1
                section: root.activeView; controller: libraryController
                queueCtrl: queueController; playbackCtrl: playbackController
            }
            FavoritesView {
                anchors.fill: parent; visible: root.activeView === "favorites"
                libraryCtrl: libraryController; likesCtrl: playlistController
                playbackCtrl: playbackController; queueCtrl: queueController
            }
            PlaylistsView {
                id: playlistsView; anchors.fill: parent
                visible: root.activeView === "playlists"
                controller: playlistController
            }
            QueueView {
                anchors.fill: parent; visible: root.activeView === "queue"
                controller: queueController; playbackCtrl: playbackController
            }
        }
    }
}
