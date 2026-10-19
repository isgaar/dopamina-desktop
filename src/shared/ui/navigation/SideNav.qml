/**
 * Location: src/shared/ui/navigation/SideNav.qml
 * Function: Stellio-inspired side navigation rail featuring 7 library sections,
 * upper collection statistics panel, and bottom shortcuts for audio tools.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import ".."

Rectangle {
    id: root
    width: 92; color: Theme.bgCard
    radius: Theme.radiusMd; clip: true
    border.color: Theme.borderDark; border.width: 1

    property string currentMode: "tracks"
    signal modeSelected(string mode)
    signal openSettingsRequested()
    signal openEqualizerRequested()
    signal openTimerRequested()

    readonly property var navItems: [
        { mode: "tracks", icon: "music", label: Strings.get("nav_tracks") },
        { mode: "albums", icon: "disc", label: Strings.get("nav_albums") },
        { mode: "artists", icon: "users", label: Strings.get("nav_artists") },
        { mode: "genres", icon: "tag", label: Strings.get("nav_genres") },
        { mode: "folders", icon: "folder", label: Strings.get("nav_folders") },
        { mode: "playlists", icon: "list",
          label: Strings.get("nav_playlists") },
        { mode: "queue", icon: "clock", label: Strings.get("nav_queue") }
    ]

    ColumnLayout {
        anchors.fill: parent; anchors.margins: 4; spacing: 2

        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 38
            radius: Theme.radiusSm; color: Theme.bgSurface
            border.color: Theme.borderDark; border.width: 1
            ColumnLayout {
                anchors.centerIn: parent; spacing: 1
                Text {
                    text: (libraryController ?
                           libraryController.totalTracks : 0) + " " +
                          Strings.get("nav_songs_suffix")
                    font.family: Theme.fontFamily; font.pixelSize: 9
                    font.bold: true; color: Theme.primary
                }
                Text {
                    text: (libraryController ?
                           libraryController.totalDurationHours.toFixed(1) :
                           "0.0") + " " + Strings.get("nav_hours_suffix")
                    font.family: Theme.fontFamily; font.pixelSize: 9
                    color: Theme.textMuted
                }
            }
        }

        Repeater {
            model: root.navItems
            delegate: Rectangle {
                Layout.fillWidth: true; Layout.preferredHeight: 44
                radius: Theme.radiusMd
                color: root.currentMode === modelData.mode ?
                    Theme.primaryDim : (itemArea.containsMouse ?
                        Theme.bgSurface : "transparent")
                ColumnLayout {
                    anchors.centerIn: parent; spacing: 1
                    AppIcon {
                        name: modelData.icon; size: 16
                        iconColor: root.currentMode === modelData.mode ?
                            Theme.primaryHover : (itemArea.containsMouse ?
                                Theme.textLight : Theme.textMuted)
                    }
                    Text {
                        text: modelData.label; font.family: Theme.fontFamily
                        font.pixelSize: 10; font.bold: true
                        color: root.currentMode === modelData.mode ?
                            Theme.primaryHover : (itemArea.containsMouse ?
                                Theme.textLight : Theme.textMuted)
                    }
                }
                MouseArea {
                    id: itemArea; anchors.fill: parent; hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.modeSelected(modelData.mode)
                }
            }
        }

        Item { Layout.fillHeight: true }

        RowLayout {
            Layout.fillWidth: true; spacing: 2
            Repeater {
                model: [
                    { i: "sliders", f: () => root.openEqualizerRequested() },
                    { i: "moon", f: () => root.openTimerRequested() },
                    { i: "settings", f: () => root.openSettingsRequested() }
                ]
                delegate: AppButton {
                    Layout.fillWidth: true; Layout.preferredHeight: 30
                    iconName: modelData.i; variant: "ghost"
                    onClicked: modelData.f()
                }
            }
        }
    }
}
