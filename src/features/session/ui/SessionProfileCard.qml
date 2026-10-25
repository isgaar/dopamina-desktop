/**
 * Location: src/features/session/ui/SessionProfileCard.qml
 * Function: Authenticated account summary with radar avatar, live Core state,
 * handoff shortcut, local security note, and logout action.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "components"
ColumnLayout {
    id: root
    spacing: Theme.spaceSm
    readonly property string connText: sessionController.serverOnline ?
        Strings.get("session_status_connected") :
        (sessionController.serverStatus === "checking" ?
            Strings.get("session_status_checking") :
            Strings.get("session_status_disconnected"))
    function trigger(action) {
        if (action === "server") sessionController.checkServer()
        else if (action === "web")
            Qt.openUrlExternally(sessionController.serverUrl)
        else if (action === "logout") sessionController.logout()
    }
    RowLayout { Layout.fillWidth: true
        spacing: Theme.spaceMd
        Item {
            Layout.preferredWidth: 84; Layout.preferredHeight: 84
            Rectangle {
                anchors.fill: parent; radius: width / 2
                color: "transparent"; border.width: 1.5
                border.color: Theme.withAlpha(Theme.accent, 0.55)
            }
            SessionAvatar { anchors.centerIn: parent
                avatarSize: 66; showStatus: false }
        }
        ColumnLayout {
            Layout.fillWidth: true; spacing: 2
            Text {
                text: sessionController.displayName
                font.family: Theme.fontFamily; font.pixelSize: 18
                font.bold: true; color: Theme.textLight
                elide: Text.ElideRight; Layout.fillWidth: true
            }
            Text {
                text: "@" + sessionController.username
                font.family: Theme.fontFamily; font.pixelSize: 11
                color: Theme.textMuted
                elide: Text.ElideRight; Layout.fillWidth: true
            }
        }
    }
    Repeater {
        model: [
            { icon: "server", action: "server",
              title: Strings.get("session_core_server"),
              subtitle: root.connText + " · " +
                  sessionController.serverHost },
            { icon: "qr-code", action: "web",
              title: Strings.get("session_link_device"),
              badge: Strings.get("session_new_badge"),
              subtitle: Strings.get("session_link_hint") },
            { icon: "shield-check", action: "",
              title: Strings.get("session_security"),
              subtitle: Strings.get("session_security_sub") },
            { icon: "log-out", action: "logout", danger: true,
              title: Strings.get("session_logout_btn"), subtitle: "" }
        ]
        delegate: SessionMenuRow {
            required property var modelData
            Layout.fillWidth: true
            iconName: modelData.icon; title: modelData.title
            subtitle: modelData.subtitle
            badge: modelData.badge || ""; danger: modelData.danger || false
            onClicked: root.trigger(modelData.action)
        }
    }
    Item { Layout.fillHeight: true }
}
