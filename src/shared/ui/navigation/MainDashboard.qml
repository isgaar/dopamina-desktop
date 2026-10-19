/**
 * Location: src/shared/ui/navigation/MainDashboard.qml
 * Function: Primary music workspace container integrating SideNav navigation
 * rail, dynamic content views, and persistent audio player controls bar.
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
import "root:/src/features/stations/ui"

Item {
    id: root
    property string activeView: "tracks"
    property bool isFloating: false
    signal openSettingsRequested()
    signal openEqualizerRequested()
    signal openTimerRequested()

    PlayerBar {
        id: playerBar; anchors.left: parent.left
        anchors.right: parent.right; anchors.bottom: parent.bottom
        anchors.margins: Theme.spaceSm; anchors.topMargin: 0
        visible: playbackController.currentTrackTitle.length > 0 ||
                 playbackController.isPlaying
    }

    SideNav {
        id: sideNav; anchors.top: parent.top; anchors.left: parent.left
        anchors.bottom: playerBar.visible ? playerBar.top : parent.bottom
        anchors.margins: Theme.spaceSm; anchors.rightMargin: 0
        currentMode: root.activeView
        onModeSelected: m => root.activeView = m
        onOpenSettingsRequested: root.openSettingsRequested()
        onOpenEqualizerRequested: root.openEqualizerRequested()
        onOpenTimerRequested: root.openTimerRequested()
    }

    Item {
        id: contentArea; anchors.top: parent.top
        anchors.left: sideNav.right; anchors.right: parent.right
        anchors.bottom: playerBar.visible ? playerBar.top : parent.bottom
        anchors.margins: Theme.spaceSm

        LibraryView {
            id: libraryView; anchors.fill: parent
            visible: ["tracks","albums","artists","genres","folders",
                      "library","home"].indexOf(root.activeView) !== -1
            section: root.activeView; controller: libraryController
            queueCtrl: queueController; playbackCtrl: playbackController
        }
        PlaylistsView {
            id: playlistsView; anchors.fill: parent
            visible: root.activeView === "playlists"
            controller: playlistController
        }
        QueueView {
            id: queueView; anchors.fill: parent
            visible: root.activeView === "queue"
            controller: queueController; playbackCtrl: playbackController
        }
        StationManagerView {
            id: managerView; anchors.fill: parent
            visible: root.activeView === "manager"
            controller: stationController; isFloating: root.isFloating
        }
    }
}
