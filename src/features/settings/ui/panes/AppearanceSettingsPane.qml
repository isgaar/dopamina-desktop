/**
 * Location: src/features/settings/ui/panes/AppearanceSettingsPane.qml
 * Function: Appearance settings pane providing interface theme language
 * choices, dynamic artwork colors, and fluid animated transition controls.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/settings"

SettingsPane {
    id: root
    title: Strings.get("settings_appearance_title")
    description: Strings.get("settings_appearance_desc")

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceMd

        SettingsSelect {
            label: Strings.get("settings_language")
            helpText: Strings.get("settings_language_help")
            options: [
                { value: "system", label: Strings.get("settings_lang_system") },
                { value: "es", label: Strings.get("settings_lang_es") },
                { value: "en", label: Strings.get("settings_lang_en") }
            ]
            currentValue: settingsController ?
                settingsController.draftLanguage : "system"
            onSelected: val => {
                if (settingsController) settingsController.draftLanguage = val
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_adaptive_color")
            checked: true
        }

        SettingsSwitchRow {
            title: Strings.get("settings_fluid_animations")
            checked: true
        }

        SettingsSwitchRow {
            title: Strings.get("settings_marquee_titles")
            checked: true
        }
    }
}
