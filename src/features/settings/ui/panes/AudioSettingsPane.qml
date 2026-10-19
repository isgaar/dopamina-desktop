/**
 * Location: src/features/settings/ui/panes/AudioSettingsPane.qml
 * Function: Audio settings pane providing controls for gapless playback, track
 * crossfading duration, volume normalization, and output sink selection.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/settings"

SettingsPane {
    id: root
    title: Strings.get("settings_audio_title")
    description: Strings.get("settings_audio_desc")

    ColumnLayout {
        Layout.fillWidth: true
        spacing: Theme.spaceMd

        SettingsSwitchRow {
            title: Strings.get("settings_gapless")
            checked: true
        }

        SettingsSwitchRow {
            id: crossfadeSwitch
            title: Strings.get("settings_crossfade_title")
            checked: false
        }

        SettingsField {
            visible: crossfadeSwitch.checked
            label: Strings.get("settings_crossfade_duration")
            text: "3"
        }

        SettingsSwitchRow {
            title: Strings.get("settings_fade_pause")
            checked: true
        }

        SettingsSelect {
            label: Strings.get("settings_norm_title")
            options: [
                { value: "off", label: "Desactivada" },
                { value: "ebu14", label: "-14 LUFS (Estándar)" },
                { value: "ebu18", label: "-18 LUFS (Hi-Fi)" },
                { value: "ebu23", label: "-23 LUFS (Broadcast)" }
            ]
            currentValue: "ebu14"
        }

        SettingsField {
            label: Strings.get("settings_buffer_size")
            text: "200"
        }
    }
}
