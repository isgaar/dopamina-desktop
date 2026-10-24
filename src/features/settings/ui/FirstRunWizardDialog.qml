/**
 * Location: src/features/settings/ui/FirstRunWizardDialog.qml
 * Function: Dedicated first-run onboarding screen guiding source selection,
 * network share credentials, live connection test spinner, and activation.
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
    color: Theme.bgCard
    radius: Theme.radiusMd; clip: true
    border.color: Theme.borderDark; border.width: 1

    readonly property string srcType: settingsController ?
        settingsController.draftSourceType : "smb"
    readonly property string status: settingsController ?
        settingsController.sourceStatus : ""
    readonly property string statusMsg: root.status === "ok" ?
        Strings.get("status_connection_success") :
        (root.status === "checking" ?
            Strings.get("status_testing_connection") :
            (root.status === "error" ?
                Strings.get("status_connection_failed") : ""))

    ColumnLayout {
        anchors.fill: parent; anchors.margins: Theme.spaceLg
        spacing: Theme.spaceMd

        Text {
            text: Strings.get("wizard_welcome_title"); font.bold: true
            font.family: Theme.fontFamily; color: Theme.textLight
            font.pixelSize: 18
        }
        Text {
            text: Strings.get("wizard_welcome_desc")
            font.family: Theme.fontFamily; color: Theme.textMuted
            font.pixelSize: 12; wrapMode: Text.WordWrap; Layout.fillWidth: true
        }

        RowLayout {
            spacing: Theme.spaceSm
            Repeater {
                model: [
                    { t: Strings.get("settings_source_smb"), m: "smb" },
                    { t: Strings.get("settings_source_folder"), m: "local" },
                    { t: Strings.get("settings_source_server"), m: "server" }
                ]
                delegate: AppButton {
                    text: modelData.t
                    variant: root.srcType === modelData.m ?
                        "primary" : "secondary"
                    onClicked: settingsController &&
                        (settingsController.draftSourceType = modelData.m)
                }
            }
        }

        FirstRunSmbFields {
            visible: root.srcType === "smb"
        }

        SettingsField {
            visible: root.srcType === "server"
            label: Strings.get("settings_server_url")
            text: settingsController ? settingsController.draftApiUrl : ""
            onTextEdited: v => settingsController &&
                (settingsController.draftApiUrl = v)
        }
        SettingsField {
            visible: root.srcType === "local"
            label: Strings.get("settings_local_path")
            text: settingsController ? settingsController.draftLocalPath : ""
            onTextEdited: v => settingsController &&
                (settingsController.draftLocalPath = v)
        }

        RowLayout {
            Layout.fillWidth: true; spacing: Theme.spaceSm
            AppButton {
                text: Strings.get("settings_btn_test_source")
                variant: "secondary"
                onClicked: settingsController &&
                    settingsController.testSourceConnection()
            }
            LoadingSpinner {
                size: 18; lineWidth: 2; visible: root.status === "checking"
            }
            Text {
                font.family: Theme.fontFamily; font.pixelSize: 11
                color: root.status === "ok" ? Theme.success :
                    (root.status === "checking" ? Theme.accent : Theme.danger)
                text: root.statusMsg; visible: text.length > 0
            }
            Item { Layout.fillWidth: true }
            AppButton {
                text: Strings.get("wizard_btn_start"); variant: "primary"
                onClicked: {
                    settingsController && settingsController.completeFirstRun()
                }
            }
        }
    }
}
