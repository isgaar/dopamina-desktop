/**
 * Location: src/features/settings/ui/panes/MusicSourceSettingsPane.qml
 * Function: Music source and scanner settings pane allowing switching between
 * remote server, local directory, and SMB storage shares, along with file format
 * filters, tag encoding, duration limits, and library re-indexing from Android.
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
                    { t: Strings.get("settings_source_local"), m: "local" }
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

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.borderDark
        }

        SettingsSwitchRow {
            title: Strings.get("settings_scan_startup_title")
            subtitle: Strings.get("settings_scan_startup_desc")
            checked: settingsController ? settingsController.getSetting("Scanner/scanOnStartup", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Scanner/scanOnStartup", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_scan_notify_title")
            subtitle: Strings.get("settings_scan_notify_desc")
            checked: settingsController ? settingsController.getSetting("Scanner/notifyNewTracks", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Scanner/notifyNewTracks", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_scan_nomedia_title")
            subtitle: Strings.get("settings_scan_nomedia_desc")
            checked: settingsController ? settingsController.getSetting("Scanner/skipNoMedia", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Scanner/skipNoMedia", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_scan_mp4_title")
            subtitle: Strings.get("settings_scan_mp4_desc")
            checked: settingsController ? settingsController.getSetting("Scanner/skipMp4", false) : false
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Scanner/skipMp4", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_scan_ogg_title")
            subtitle: Strings.get("settings_scan_ogg_desc")
            checked: settingsController ? settingsController.getSetting("Scanner/skipOgg", false) : false
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Scanner/skipOgg", isChecked)
            }
        }

        SettingsSelect {
            label: Strings.get("settings_scan_encoding_title")
            helpText: Strings.get("settings_scan_encoding_desc")
            options: [
                { value: "utf8", label: "Auto (UTF-8)" },
                { value: "latin1", label: "ISO-8859-1 (Latin-1)" }
            ]
            currentValue: settingsController ? settingsController.getSetting("Scanner/encoding", "utf8") : "utf8"
            onSelected: val => {
                if (settingsController) settingsController.setSetting("Scanner/encoding", val)
            }
        }

        SettingsSelect {
            label: Strings.get("settings_scan_short_title")
            helpText: Strings.get("settings_scan_short_desc")
            options: [
                { value: "0", label: "Incluir todo" },
                { value: "5", label: "< 5 s" },
                { value: "15", label: "< 15 s" },
                { value: "30", label: "< 30 s" }
            ]
            currentValue: settingsController ? String(settingsController.getSetting("Scanner/skipShortSec", "5")) : "5"
            onSelected: val => {
                if (settingsController) settingsController.setSetting("Scanner/skipShortSec", val)
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            AppButton {
                text: Strings.get("settings_scan_delete_db_title")
                variant: "secondary"
                onClicked: {
                    if (settingsController) {
                        settingsController.rescanLibrary()
                        rescanStatusText.text = Strings.get("settings_scan_rescan_done")
                    }
                }
            }

            Text {
                id: rescanStatusText
                text: Strings.get("settings_scan_delete_db_desc")
                font.family: Theme.fontFamily
                font.pixelSize: 11
                color: Theme.textDim
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }
    }
}
