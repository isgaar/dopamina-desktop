/**
 * Location: src/shared/ui/titlebar/AppTitleBar.qml
 * Function: Wide application header pairing the Dopamine wordmark with a
 * centered library search, account avatar, and desktop window controls.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Window
import "root:/src/shared"
import "root:/src/features/session/ui"

Rectangle {
    id: root
    height: 56
    color: "transparent"
    clip: true

    property var window: null
    property bool isFirstRun: false
    signal searchEdited(string text)
    signal openSessionRequested()
    signal openSettingsRequested()

    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width; height: 1
        color: Theme.borderDark
    }

    MouseArea {
        anchors.fill: parent
        onPressed: if (root.window) root.window.startSystemMove()
        onDoubleClicked: if (root.window && !root.isFirstRun) {
            root.window.visibility === Window.Maximized ?
                root.window.showNormal() : root.window.showMaximized()
        }
    }

    AppBrand {
        anchors.left: parent.left
        anchors.leftMargin: Theme.spaceMd
        anchors.verticalCenter: parent.verticalCenter
    }

    TopSearch {
        id: search
        height: 34
        width: Math.min(400, root.width * 0.38)
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        visible: !root.isFirstRun
        onEdited: text => root.searchEdited(text)
    }

    WindowControls {
        id: windowControls
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: Theme.spaceMd
        window: root.window
        visible: Platform.useCustomTitleBar
        canMaximize: !root.isFirstRun
    }

    SessionAvatar {
        id: avatar
        avatarSize: 32
        visible: !root.isFirstRun
        anchors.right: windowControls.visible ?
            windowControls.left : parent.right
        anchors.rightMargin: 10
        anchors.verticalCenter: parent.verticalCenter
        onClicked: root.openSessionRequested()
    }

    Rectangle {
        id: settingsBtn
        width: 32; height: 32; radius: 16
        color: settingsHover.containsMouse ? Theme.bgSurface : "transparent"
        visible: !root.isFirstRun
        anchors.right: avatar.left
        anchors.rightMargin: 8
        anchors.verticalCenter: parent.verticalCenter

        AppIcon {
            anchors.centerIn: parent
            name: "settings"
            size: 17
            iconColor: settingsHover.containsMouse ? Theme.textLight : Theme.textMuted
        }

        MouseArea {
            id: settingsHover
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: root.openSettingsRequested()
        }
    }
}
