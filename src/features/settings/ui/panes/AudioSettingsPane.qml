/**
 * Location: src/features/settings/ui/panes/AudioSettingsPane.qml
 * Function: Audio settings pane providing controls for gapless playback, track
 * crossfading duration, volume fading, silence skipping, volume stabilization,
 * visualizer animation, normalization, and buffer size matching Android options.
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
            subtitle: Strings.get("settings_gapless_desc")
            checked: settingsController ? settingsController.getSetting("Audio/gapless", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Audio/gapless", isChecked)
                if (typeof audioSettings !== "undefined" && audioSettings) {
                    audioSettings.gaplessEnabled = isChecked
                }
            }
        }

        SettingsSwitchRow {
            id: crossfadeSwitch
            title: Strings.get("settings_crossfade_title")
            subtitle: Strings.get("settings_crossfade_track_desc")
            checked: settingsController ? settingsController.getSetting("Audio/crossfadeOn", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Audio/crossfadeOn", isChecked)
            }
        }

        SettingsField {
            visible: crossfadeSwitch.checked
            label: Strings.get("settings_crossfade_duration") + " (s)"
            text: settingsController ? String(settingsController.getSetting("Audio/crossfadeSec", 3)) : "3"
            onTextEdited: val => {
                var num = parseFloat(val) || 3.0
                if (settingsController) settingsController.setSetting("Audio/crossfadeSec", num)
                if (typeof audioSettings !== "undefined" && audioSettings) {
                    audioSettings.crossfadeSec = num
                }
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_fade_pause")
            subtitle: Strings.get("settings_fade_play_desc")
            checked: settingsController ? settingsController.getSetting("Audio/fadePlayPause", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Audio/fadePlayPause", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_skip_silence_title")
            subtitle: Strings.get("settings_skip_silence_desc")
            checked: settingsController ? settingsController.getSetting("Audio/skipSilence", false) : false
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Audio/skipSilence", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_volume_stabilizer_title")
            subtitle: Strings.get("settings_volume_stabilizer_desc")
            checked: settingsController ? settingsController.getSetting("Audio/volumeStabilizer", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Audio/volumeStabilizer", isChecked)
            }
        }

        SettingsSwitchRow {
            title: Strings.get("settings_visualizer_title")
            subtitle: Strings.get("settings_visualizer_desc")
            checked: settingsController ? settingsController.getSetting("Audio/visualizerEnabled", true) : true
            onToggled: isChecked => {
                if (settingsController) settingsController.setSetting("Audio/visualizerEnabled", isChecked)
            }
        }

        SettingsSelect {
            label: Strings.get("settings_norm_title")
            options: [
                { value: "off", label: "Desactivada" },
                { value: "ebu14", label: "-14 LUFS (Estándar)" },
                { value: "ebu18", label: "-18 LUFS (Hi-Fi)" },
                { value: "ebu23", label: "-23 LUFS (EBU R128)" }
            ]
            currentValue: settingsController ? settingsController.getSetting("Audio/normalization", "ebu14") : "ebu14"
            onSelected: val => {
                if (settingsController) settingsController.setSetting("Audio/normalization", val)
            }
        }

        SettingsSelect {
            label: Strings.get("settings_buffer_size")
            helpText: Strings.get("settings_buffer_size_desc")
            options: [
                { value: "500", label: "500 ms" },
                { value: "1000", label: "1000 ms" },
                { value: "2000", label: "2000 ms" }
            ]
            currentValue: settingsController ? String(settingsController.getSetting("Audio/bufferSizeMs", "1000")) : "1000"
            onSelected: val => {
                var ms = parseInt(val) || 1000
                if (settingsController) settingsController.setSetting("Audio/bufferSizeMs", ms)
                if (typeof audioSettings !== "undefined" && audioSettings) {
                    audioSettings.bufferSizeMs = ms
                }
            }
        }
    }
}
