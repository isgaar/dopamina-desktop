/**
 * Location: src/features/settings/ui/panes/CoversSettingsPane.qml
 * Function: Covers settings pane managing album artwork download preferences,
 * carousel effects, hidden files without extension, resolution, quality,
 * synchronized lyrics retrieval, and cache clearing matching Android options.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/settings"

SettingsPane {
    id: root
    title: Strings.get("settings_covers_title")
    description: Strings.get("settings_covers_desc")

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceMd

        SettingsSwitchRow {
            title: Strings.get("settings_carousel_title")
            subtitle: Strings.get("settings_carousel_desc")
            checked: settingsController ? settingsController.getSetting("Covers/carouselEnabled", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Covers/carouselEnabled", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_download_art")
            checked: settingsController ? settingsController.getSetting("Covers/downloadMissing", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Covers/downloadMissing", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_covers_wifi")
            checked: settingsController ? settingsController.getSetting("Covers/wifiOnly", false) : false
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Covers/wifiOnly", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_prefer_folder_art")
            checked: settingsController ? settingsController.getSetting("Covers/preferFolderArt", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Covers/preferFolderArt", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_cover_no_ext_title")
            subtitle: Strings.get("settings_cover_no_ext_desc")
            checked: settingsController ? settingsController.getSetting("Covers/saveWithoutExtension", false) : false
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Covers/saveWithoutExtension", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_auto_lrc")
            checked: settingsController ? settingsController.getSetting("Covers/autoLrc", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Covers/autoLrc", isChecked)
            }
        }

        SettingsSelect {
            label: Strings.get("settings_cover_quality")
            options: [
                { value: "100", label: "100% (Máxima fidelidad)" },
                { value: "80", label: "80% (Equilibrada)" },
                { value: "60", label: "60% (Ahorro de espacio)" }
            ]
            currentValue: settingsController ? String(settingsController.getSetting("Covers/quality", "80")) : "80"
            onSelected: val => {
                if (settingsController) settingsController.setSetting("Covers/quality", val)
            }
        }

        SettingsSelect {
            label: Strings.get("settings_cover_size")
            helpText: Strings.get("settings_cover_size_desc")
            options: [
                { value: "300", label: "300 px" },
                { value: "500", label: "500 px" },
                { value: "1000", label: "1000 px" }
            ]
            currentValue: settingsController ? String(settingsController.getSetting("Covers/maxSize", "500")) : "500"
            onSelected: val => {
                if (settingsController) settingsController.setSetting("Covers/maxSize", val)
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.borderDark
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            AppButton {
                text: Strings.get("settings_cover_clear_title")
                variant: "danger"
                onClicked: {
                    if (settingsController) {
                        settingsController.clearCoversCache()
                        cacheStatusText.text = Strings.get("settings_cover_clear_done")
                    }
                }
            }

            Text {
                id: cacheStatusText
                text: Strings.get("settings_cover_clear_desc")
                font.family: Theme.fontFamily
                font.pixelSize: 11
                color: Theme.textDim
                Layout.fillWidth: true
                elide: Text.ElideRight
            }
        }
    }
}
