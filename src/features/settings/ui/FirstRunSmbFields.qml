/**
 * Location: src/features/settings/ui/FirstRunSmbFields.qml
 * Function: First-run setup form fields configuring SMB server, share, folder,
 * user credentials, and password persistence in secure system keychain.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui/settings"

ColumnLayout {
    id: root
    spacing: Theme.spaceSm
    Layout.fillWidth: true

    function parseSmbHost(raw) {
        var v = raw.trim();
        if (v.indexOf("smb://") === 0) {
            var parts = v.substring(6).split("/").filter(p => p.length > 0);
            if (parts.length > 0) settingsController.draftSmbHost = parts[0];
            if (parts.length > 1) settingsController.draftSmbShare = parts[1];
            if (parts.length > 2)
                settingsController.draftSmbFolder = parts.slice(2).join("/");
        } else if (settingsController) {
            settingsController.draftSmbHost = v;
        }
    }

    RowLayout {
        Layout.fillWidth: true; spacing: Theme.spaceSm
        SettingsField {
            Layout.fillWidth: true; monospace: true
            label: Strings.get("settings_smb_host")
            placeholderText: "192.168.0.2 o smb://..."
            text: settingsController ? settingsController.draftSmbHost : ""
            onTextEdited: v => root.parseSmbHost(v)
        }
        SettingsField {
            Layout.fillWidth: true; monospace: true
            label: Strings.get("settings_smb_share")
            placeholderText: "datos"
            text: settingsController ? settingsController.draftSmbShare : ""
            onTextEdited: v => settingsController &&
                (settingsController.draftSmbShare = v)
        }
    }

    RowLayout {
        Layout.fillWidth: true; spacing: Theme.spaceSm
        SettingsField {
            Layout.fillWidth: true; monospace: true
            label: Strings.get("settings_smb_folder")
            placeholderText: "BackupMusic"
            text: settingsController ? settingsController.draftSmbFolder : ""
            onTextEdited: v => settingsController &&
                (settingsController.draftSmbFolder = v)
        }
        SettingsField {
            Layout.fillWidth: true
            label: Strings.get("settings_smb_user")
            text: settingsController ? settingsController.draftSmbUser : ""
            onTextEdited: v => settingsController &&
                (settingsController.draftSmbUser = v)
        }
    }

    RowLayout {
        Layout.fillWidth: true; spacing: Theme.spaceSm
        SettingsField {
            Layout.fillWidth: true
            label: Strings.get("settings_smb_password")
            echoMode: TextInput.Password
            text: settingsController ? settingsController.draftSmbPassword : ""
            onTextEdited: v => settingsController &&
                (settingsController.draftSmbPassword = v)
        }
        SettingsSwitchRow {
            title: Strings.get("settings_smb_save_creds")
            checked: settingsController ?
                settingsController.draftSaveCredentials : true
            onToggled: c => settingsController &&
                (settingsController.draftSaveCredentials = c)
        }
    }
}
