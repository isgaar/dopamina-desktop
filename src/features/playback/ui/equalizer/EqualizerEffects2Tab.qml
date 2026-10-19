/**
 * Location: src/features/playback/ui/equalizer/EqualizerEffects2Tab.qml
 * Function: Stellio Effects2 tab managing pitch shift, playback speed,
 * echo, autowah, reverb dials, flanger toggle, and reset action.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

ColumnLayout {
    id: root; spacing: Theme.spaceMd
    Rectangle {
        Layout.fillWidth: true; Layout.preferredHeight: 116
        radius: Theme.radiusMd; color: Theme.bgCard
        border.color: Theme.borderDark; border.width: 1
        ColumnLayout {
            anchors.fill: parent; anchors.margins: Theme.spaceSm; spacing: 4
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: Strings.get("eq_pitch"); font.pixelSize: 11
                    font.bold: true; color: Theme.textMuted
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: (equalizerController.pitch > 0 ? "+" : "") +
                        equalizerController.pitch.toFixed(1) + " st"
                    font.pixelSize: 11; font.bold: true
                    color: equalizerController.pitch !== 0 ?
                        Theme.color("eq_coral") : Theme.textDim
                }
            }
            Slider {
                Layout.fillWidth: true; from: -12.0; to: 12.0; stepSize: 0.5
                value: equalizerController.pitch
                onMoved: equalizerController.setPitch(value)
            }
            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: Strings.get("eq_speed"); font.pixelSize: 11
                    font.bold: true; color: Theme.textMuted
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: "x" + equalizerController.speed.toFixed(2)
                    font.pixelSize: 11; font.bold: true
                    color: equalizerController.speed !== 1.0 ?
                        Theme.color("eq_coral") : Theme.textDim
                }
            }
            Slider {
                Layout.fillWidth: true; from: 0.3; to: 3.0; stepSize: 0.05
                value: equalizerController.speed
                onMoved: equalizerController.setSpeed(value)
            }
        }
    }

    GridLayout {
        Layout.fillWidth: true; columns: 2
        rowSpacing: Theme.spaceSm; columnSpacing: Theme.spaceSm

        Repeater {
            model: [
                { t: Strings.get("eq_echo"), v: equalizerController.echo,
                  a: equalizerController.echoActive },
                { t: Strings.get("eq_autowah"), v: equalizerController.autoWah,
                  a: equalizerController.autoWahActive },
                { t: Strings.get("eq_reverb"), v: equalizerController.reverb,
                  a: equalizerController.reverbActive }
            ]
            EqualizerKnob {
                Layout.alignment: Qt.AlignHCenter
                title: modelData.t; value: modelData.v; active: modelData.a
                onValueChangedByUser: (v) =>
                    equalizerController.setEffect2Knob(index, v)
                onToggledByUser: (en) =>
                    equalizerController.toggleEffect2Knob(index, en)
            }
        }

        Rectangle {
            Layout.preferredWidth: 120; Layout.preferredHeight: 126
            radius: Theme.radiusMd; color: Theme.bgCard
            border.color: Theme.borderDark; border.width: 1

            ColumnLayout {
                anchors.centerIn: parent; spacing: 8
                AppButton {
                    Layout.preferredWidth: 100; Layout.preferredHeight: 32
                    text: Strings.get("eq_flanger"); iconName: "power"
                    variant: equalizerController.flanger ?
                        "primary" : "secondary"
                    onClicked: equalizerController.setFlanger(
                        !equalizerController.flanger)
                }
                AppButton {
                    Layout.preferredWidth: 100; Layout.preferredHeight: 32
                    text: Strings.get("eq_reset"); iconName: "rotate-ccw"
                    variant: "secondary"
                    onClicked: equalizerController.resetEffects2()
                }
            }
        }
    }
}
