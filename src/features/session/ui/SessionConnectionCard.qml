/**
 * Location: src/features/session/ui/SessionConnectionCard.qml
 * Function: Core server card with address input, live status capsule, and
 * manual connection probe matching the Android session screen.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/feedback"
import "root:/src/shared/ui/settings"
Rectangle {
    id: root
    readonly property string status: sessionController.serverStatus
    implicitHeight: column.implicitHeight + Theme.spaceMd * 2
    radius: Theme.radiusLg
    color: Theme.bgCard
    border.color: Theme.borderDark
    border.width: 1
    ColumnLayout {
        id: column
        anchors.fill: parent
        anchors.margins: Theme.spaceMd
        spacing: Theme.spaceSm
        RowLayout {
            Layout.fillWidth: true; spacing: Theme.spaceSm
            AppIcon { name: "wifi"; size: 15
                iconColor: Theme.primary }
            Text {
                text: Strings.get("session_core_server")
                font.family: Theme.fontFamily; font.pixelSize: 13
                font.bold: true; color: Theme.textLight
            }
            Item { Layout.fillWidth: true }
            StatusPill {
                variant: root.status === "online" ? "ok" :
                    (root.status === "checking" ? "warn" : "error")
                text: root.status === "online" ?
                    Strings.get("session_status_connected") :
                    (root.status === "checking" ?
                        Strings.get("session_status_checking") :
                        Strings.get("session_status_disconnected"))
            }
        }
        SettingsField {
            id: serverField
            Layout.fillWidth: true
            label: Strings.get("session_server_url")
            placeholderText: Strings.get("session_server_url_hint")
            text: sessionController.serverDraft
            onTextEdited: v => sessionController.serverDraft = v
        }
        RowLayout {
            Layout.fillWidth: true; spacing: Theme.spaceSm
            AppButton {
                text: Strings.get("session_test_connection")
                iconName: "refresh-cw"; variant: "secondary"
                enabled: !sessionController.busy
                onClicked: sessionController.checkServer()
            }
            LoadingSpinner {
                size: 14; lineWidth: 2
                visible: sessionController.busy
            }
            Item { Layout.fillWidth: true }
        }
    }
    Connections {
        target: sessionController
        function onServerDraftChanged() {
            serverField.text = sessionController.serverDraft
        }
    }
}
