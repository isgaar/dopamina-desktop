/**
 * Location: src/features/session/ui/SessionLoginForm.qml
 * Function: Desktop sign-in form with Core address, credentials, and the web
 * console handoff shortcut that transfers an active browser session.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/settings"
ColumnLayout {
    id: root
    spacing: Theme.spaceMd
    SessionConnectionCard { Layout.fillWidth: true }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: creds.implicitHeight + Theme.spaceMd * 2
        radius: Theme.radiusLg; color: Theme.bgCard
        border.color: Theme.borderDark; border.width: 1
        ColumnLayout {
            id: creds
            anchors.fill: parent; anchors.margins: Theme.spaceMd
            spacing: Theme.spaceSm
            Text {
                text: Strings.get("session_login_title")
                font.family: Theme.fontFamily; font.pixelSize: 14
                font.bold: true; color: Theme.textLight
            }
            SettingsField {
                id: userField; Layout.fillWidth: true
                label: Strings.get("session_username")
                placeholderText: Strings.get("session_username")
            }
            SettingsField {
                id: passField; Layout.fillWidth: true
                label: Strings.get("session_password")
                placeholderText: Strings.get("session_password")
                echoMode: TextInput.Password
            }
            AppButton {
                Layout.fillWidth: true; Layout.preferredHeight: 38
                text: Strings.get("session_login_btn")
                iconName: "log-in"
                enabled: !sessionController.busy &&
                    userField.text.trim().length > 0
                onClicked: sessionController.login(
                    userField.text, passField.text)
            }
        }
    }
    Rectangle {
        Layout.fillWidth: true
        implicitHeight: hint.implicitHeight + Theme.spaceSm * 2
        radius: Theme.radiusLg; color: Theme.withAlpha(Theme.accent, 0.06)
        border.color: Theme.withAlpha(Theme.accent, 0.25); border.width: 1
        RowLayout {
            id: hint
            anchors.fill: parent; anchors.margins: Theme.spaceSm
            spacing: Theme.spaceSm
            AppIcon { name: "qr-code"; size: 18; iconColor: Theme.accent }
            ColumnLayout {
                Layout.fillWidth: true; spacing: 1
                Text {
                    text: Strings.get("session_link_hint")
                    font.family: Theme.fontFamily; font.pixelSize: 10
                    color: Theme.textMuted; wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
            }
            AppButton {
                text: Strings.get("session_open_web")
                iconName: "external-link"; variant: "ghost"
                onClicked: Qt.openUrlExternally(
                    sessionController.serverDraft)
            }
        }
    }
    Item { Layout.fillHeight: true }
}
