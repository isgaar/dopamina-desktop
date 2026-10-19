/**
 * Location: src/shared/ui/AppMenuBar.qml
 * Function: Native platform global menu bar integrating with KDE Plasma and
 * desktop environments via DBus, providing session and window commands.
 * All Rights Reserved isgaar
 */
import QtQuick
import Qt.labs.platform as Platform

Platform.MenuBar {
    id: root
    signal openAboutRequested()
    signal openSettingsRequested()
    signal openEqualizerRequested()

    Platform.Menu {
        title: Strings.get("menu_file")
        Platform.MenuItem {
            text: Strings.get("menu_equalizer")
            onTriggered: root.openEqualizerRequested()
        }
        Platform.MenuItem {
            text: Strings.get("menu_settings")
            shortcut: StandardKey.Preferences
            onTriggered: root.openSettingsRequested()
        }
        Platform.MenuSeparator {}
        Platform.MenuItem {
            text: Strings.get("menu_exit")
            role: Platform.MenuItem.QuitRole
            shortcut: StandardKey.Quit
            onTriggered: Qt.quit()
        }
    }

    Platform.Menu {
        title: Strings.get("menu_help")
        Platform.MenuItem {
            text: Strings.get("menu_about")
            role: Platform.MenuItem.AboutRole
            onTriggered: root.openAboutRequested()
        }
    }
}
