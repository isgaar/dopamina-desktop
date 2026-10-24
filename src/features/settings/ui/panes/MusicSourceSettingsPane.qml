/**
 * Location: src/features/settings/ui/panes/MusicSourceSettingsPane.qml
 * Function: Music source settings pane allowing switching between remote
 * server, local directory, and SMB storage shares with credential persistence.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/settings"

SettingsPane {
    id: root
    title: Strings.get("settings_source_title")
    description: Strings.get("settings_source_desc")
    readonly property string srcType: settingsController ?
        settingsController.draftSourceType : ""

    function statusText(s) {
        return s === "ok" ? Strings.get("settings_source_status_ok") :
               s === "checking" ? Strings.get("settings_status_checking") :
               s === "error" ? Strings.get("settings_status_disconnected") :
               Strings.get("settings_source_status_untested");
    }

    ColumnLayout {
        Layout.fillWidth: true; spacing: Theme.spaceMd

        RowLayout {
            spacing: Theme.spaceSm
            Repeater {
                model: [
                    { t: Strings.get("settings_source_server"), m: "server" },
                    { t: Strings.get("settings_source_local"), m: "local" },
                    { t: Strings.get("settings_source_smb"), m: "smb" }
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
        SettingsField {
            visible: root.srcType === "smb"
            label: Strings.get("settings_smb_host")
            text: settingsController ? settingsController.draftSmbHost : ""
            onTextEdited: v => settingsController &&
                (settingsController.draftSmbHost = v)
        }
        SettingsField {
            visible: root.srcType === "smb"
            label: Strings.get("settings_smb_share")
            text: settingsController ? settingsController.draftSmbShare : ""
            onTextEdited: v => settingsController &&
                (settingsController.draftSmbShare = v)
        }
        SettingsField {
            visible: root.srcType === "smb"
            label: Strings.get("settings_smb_folder")
            text: settingsController ? settingsController.draftSmbFolder : ""
            onTextEdited: v => settingsController &&
                (settingsController.draftSmbFolder = v)
        }
        SettingsField {
            visible: root.srcType === "smb"
            label: Strings.get("settings_smb_user")
            text: settingsController ? settingsController.draftSmbUser : ""
            onTextEdited: v => settingsController &&
                (settingsController.draftSmbUser = v)
        }
        SettingsField {
            visible: root.srcType === "smb"
            label: Strings.get("settings_smb_password")
            text: settingsController ? settingsController.draftSmbPassword : ""
            onTextEdited: v => settingsController &&
                (settingsController.draftSmbPassword = v)
        }

        RowLayout {
            Layout.fillWidth: true; spacing: Theme.spaceSm
            AppButton {
                text: Strings.get("settings_btn_test_source")
                variant: "secondary"
                onClicked: settingsController &&
                    settingsController.testSourceConnection()
            }
            StatusPill {
                property string st: settingsController ?
                    settingsController.sourceStatus : "neutral"
                variant: st === "ok" ? "ok" :
                    (st === "checking" ? "warn" : "error")
                text: root.statusText(st)
            }
            Item { Layout.fillWidth: true }
        }
    }
}
