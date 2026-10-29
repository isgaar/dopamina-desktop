/**
 * Location: src/features/settings/ui/panes/AppearanceSettingsPane.qml
 * Function: Appearance settings pane providing the interface language,
 * dynamic artwork accent color extraction, and the five XML-backed
 * Android theme palettes with live preview swatches.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/settings"
import "root:/src/features/settings/ui"

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
                { value: "system",
                  label: Strings.get("settings_lang_system") },
                { value: "es", label: Strings.get("settings_lang_es") },
                { value: "en", label: Strings.get("settings_lang_en") }
            ]
            currentValue: settingsController ?
                settingsController.draftLanguage : "system"
            onSelected: val => {
                if (settingsController)
                    settingsController.draftLanguage = val
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_accent_dynamic")
            subtitle: Strings.get("settings_accent_dynamic_desc")
            checked: settingsController ? settingsController.getSetting("Appearance/dynamicAccent", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Appearance/dynamicAccent", isChecked)
            }
        }

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            Text {
                text: Strings.get("settings_theme")
                font.family: Theme.fontFamily
                font.pixelSize: 12
                font.bold: true
                color: Theme.textLight
            }

            Text {
                text: Strings.get("settings_theme_help")
                font.family: Theme.fontFamily
                font.pixelSize: 11
                color: Theme.textDim
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            ThemePicker {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spaceXs
                currentTheme: settingsController ?
                    settingsController.draftTheme : "roseglow"
                onThemeSelected: themeId => {
                    if (settingsController)
                        settingsController.draftTheme = themeId
                }
            }
        }
    }
}
