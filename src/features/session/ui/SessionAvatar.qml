/**
 * Location: src/features/session/ui/SessionAvatar.qml
 * Function: Compact titlebar account avatar with signed-in portrait, fallback
 * initial, and live Core connectivity dot mirroring the Android client.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import "root:/src/shared"
import "root:/src/shared/ui"
Item {
    id: root
    property int avatarSize: 26
    property bool showStatus: true
    signal clicked()
    implicitWidth: avatarSize
    implicitHeight: avatarSize
    scale: hover.hovered ? 1.06 : 1.0
    Behavior on scale {
        NumberAnimation { duration: 140; easing.type: Easing.OutCubic }
    }
    Rectangle {
        anchors.fill: parent
        radius: width / 2
        color: sessionController.isLoggedIn ? Theme.bgSurface :
                                              Theme.bgDark
        border.width: 1.2
        border.color: sessionController.isLoggedIn ?
            Theme.withAlpha(Theme.primary, 0.85) : Theme.borderDark
    }
    RoundedImage {
        anchors.fill: parent
        anchors.margins: 1.5
        radius: width / 2
        source: sessionController.isLoggedIn ?
            sessionController.avatarUrl : ""
    }
    Text {
        anchors.centerIn: parent
        visible: sessionController.isLoggedIn &&
            sessionController.avatarUrl.length === 0
        text: sessionController.initial
        color: Theme.textLight
        font.family: Theme.fontFamily
        font.pixelSize: Math.round(root.avatarSize * 0.42)
        font.bold: true
    }
    AppIcon {
        anchors.centerIn: parent
        visible: !sessionController.isLoggedIn
        name: "user-round"
        size: Math.round(root.avatarSize * 0.55)
        iconColor: Theme.textMuted
    }
    Rectangle {
        visible: root.showStatus
        width: 8; height: 8; radius: 4
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        color: sessionController.serverOnline ? Theme.success :
                                                Theme.textDim
        border.width: 1.5
        border.color: Theme.bgDarkest
    }
    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
    ToolTip {
        visible: hover.containsMouse
        delay: 600
        text: Strings.get("session_avatar_title")
    }
}
