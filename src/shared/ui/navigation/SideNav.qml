/**
 * Location: src/shared/ui/navigation/SideNav.qml
 * Function: Android-inspired navigation rail with grouped destinations,
 * playlist actions, tools, and a collapsible icon-only mode.
 * All Rights Reserved isgaar
 */
import QtQuick
import ".."

Item {
    id: root
    property bool collapsed: settingsController ?
        settingsController.sidebarCollapsed : false
    property int targetWidth: settingsController ?
        settingsController.sidebarWidth : 240
    property int lastExpandedWidth: 240
    property bool animating: true
    property string currentMode: "tracks"
    signal modeSelected(string mode)
    signal newPlaylistRequested()
    signal openSettingsRequested()
    signal openEqualizerRequested()
    signal openTimerRequested()
    signal openAboutRequested()
    readonly property int inset: collapsed ? Theme.spaceSm : Theme.spaceMd
    width: collapsed ? 64 : targetWidth

    Behavior on width {
        enabled: root.animating
        SpringAnimation { spring: 3.2; damping: 1.0 }
    }

    readonly property var navItems: [
        { mode: "tracks", icon: "house", label: Strings.get("nav_principal") },
        { mode: "genres", icon: "compass", label: Strings.get("nav_explore") },
        { mode: "folders", icon: "layers",
          label: Strings.get("nav_local_library") }
    ]

    Column {
        id: destinations
        anchors.top: parent.top; anchors.topMargin: Theme.spaceMd
        anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: root.inset; anchors.rightMargin: root.inset
        spacing: Theme.spaceSm

        Text {
            visible: !root.collapsed
            text: Strings.get("nav_section_library")
            font.family: Theme.fontFamily; font.pixelSize: 11
            font.bold: true; color: Theme.textDim
            leftPadding: Theme.spaceXs
        }

        Repeater {
            model: root.navItems
            delegate: SideNavItem {
                width: destinations.width; collapsed: root.collapsed
                iconName: modelData.icon; label: modelData.label
                active: root.currentMode === modelData.mode
                onClicked: root.modeSelected(modelData.mode)
            }
        }
    }

    SideNavPlaylistCard {
        id: playlistCard
        anchors.top: destinations.bottom; anchors.topMargin: Theme.spaceMd
        anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: root.inset; anchors.rightMargin: root.inset
        collapsed: root.collapsed
        onNewPlaylistRequested: root.newPlaylistRequested()
    }

    SideNavLikedCard {
        anchors.top: playlistCard.bottom
        anchors.topMargin: Theme.spaceSm + Theme.spaceXs
        anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: root.inset; anchors.rightMargin: root.inset
        collapsed: root.collapsed
        onLikedMusicRequested: root.modeSelected("favorites")
    }

    SideNavTools {
        anchors.left: parent.left; anchors.right: parent.right
        anchors.leftMargin: root.inset; anchors.rightMargin: root.inset
        anchors.bottom: parent.bottom; anchors.bottomMargin: Theme.spaceSm
        collapsed: root.collapsed
        onCollapseRequested: {
            root.animating = true; root.collapsed = !root.collapsed
            if (settingsController)
                settingsController.sidebarCollapsed = root.collapsed
        }
        onEqualizerRequested: root.openEqualizerRequested()
        onTimerRequested: root.openTimerRequested()
        onAboutRequested: root.openAboutRequested()
        onSettingsRequested: root.openSettingsRequested()
    }
}
