/**
 * Location: src/shared/ui/titlebar/TitleMenuBar.qml
 * Function: Integrated horizontal menu bar embedded inside the custom title
 * bar with localized popups, keyboard shortcuts, and dark theme design tokens.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls

MenuBar {
    id: root
    signal openAboutRequested()
    signal openSettingsRequested()
    signal openEqualizerRequested()

    padding: 0
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0

    background: Rectangle {
        color: "transparent"
    }

    delegate: MenuBarItem {
        id: menuBarItem
        implicitHeight: 26
        leftPadding: Theme.spaceSm
        rightPadding: Theme.spaceSm
        topPadding: 0
        bottomPadding: 0

        contentItem: Text {
            text: menuBarItem.text
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            color: menuBarItem.highlighted ? Theme.textLight : Theme.textMuted
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
        background: Rectangle {
            color: menuBarItem.highlighted ? Theme.bgSurface : "transparent"
            radius: Theme.radiusSm
            anchors.fill: parent
            anchors.topMargin: 2
            anchors.bottomMargin: 2
        }
    }

    Menu {
        title: Strings.get("menu_file")
        Action {
            text: Strings.get("menu_equalizer")
            onTriggered: root.openEqualizerRequested()
        }
        Action {
            text: Strings.get("menu_settings")
            onTriggered: root.openSettingsRequested()
        }
        MenuSeparator {}
        Action {
            text: Strings.get("menu_exit")
            onTriggered: Qt.quit()
        }
    }

    Menu {
        title: Strings.get("menu_help")
        Action {
            text: Strings.get("menu_about")
            onTriggered: root.openAboutRequested()
        }
    }
}
