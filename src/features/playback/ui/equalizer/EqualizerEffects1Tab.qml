/**
 * Location: src/features/playback/ui/equalizer/EqualizerEffects1Tab.qml
 * Function: Stellio Effects1 tab offering AGC toggle, reset action, stereo
 * balance slider, and rotary dials for Z-Bass, Compressor, Z-Mids, and Treble.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

ColumnLayout {
    id: root
    spacing: Theme.spaceMd

    RowLayout {
        Layout.fillWidth: true; spacing: Theme.spaceSm

        AppButton {
            Layout.preferredWidth: 120; Layout.preferredHeight: 32
            text: Strings.get("eq_agc"); iconName: "power"
            variant: equalizerController.agc ? "primary" : "secondary"
            onClicked: equalizerController.setAgc(!equalizerController.agc)
        }

        Item { Layout.fillWidth: true }

        AppButton {
            Layout.preferredWidth: 120; Layout.preferredHeight: 32
            text: Strings.get("eq_reset"); iconName: "rotate-ccw"
            variant: "secondary"
            onClicked: equalizerController.resetEffects1()
        }
    }

    Rectangle {
        Layout.fillWidth: true; Layout.preferredHeight: 62
        radius: Theme.radiusMd; color: Theme.bgCard
        border.color: Theme.borderDark; border.width: 1

        ColumnLayout {
            anchors.fill: parent; anchors.margins: Theme.spaceSm
            spacing: 2

            RowLayout {
                Layout.fillWidth: true
                Text {
                    text: Strings.get("eq_balance"); font.pixelSize: 11
                    font.bold: true; color: Theme.textMuted
                }
                Item { Layout.fillWidth: true }
                Text {
                    text: Math.round(equalizerController.balance) === 0 ? "0" :
                        (equalizerController.balance < 0 ?
                         "L " + Math.abs(Math.round(
                             equalizerController.balance)) :
                         "R " + Math.round(equalizerController.balance))
                    font.pixelSize: 11; font.bold: true
                    color: equalizerController.balance !== 0 ?
                        Theme.color("eq_coral") : Theme.textDim
                }
            }

            RowLayout {
                Layout.fillWidth: true; spacing: 8
                Text { text: "L"; font.pixelSize: 11; color: Theme.textDim }
                Slider {
                    Layout.fillWidth: true; from: -100.0; to: 100.0
                    stepSize: 1.0; value: equalizerController.balance
                    onMoved: equalizerController.setBalance(value)
                }
                Text { text: "R"; font.pixelSize: 11; color: Theme.textDim }
            }
        }
    }

    GridLayout {
        Layout.fillWidth: true; columns: 2
        rowSpacing: Theme.spaceSm; columnSpacing: Theme.spaceSm

        Repeater {
            model: [
                { t: Strings.get("eq_z_bass"), v: equalizerController.zBass,
                  a: equalizerController.zBassActive },
                { t: Strings.get("eq_compressor"),
                  v: equalizerController.compressor,
                  a: equalizerController.compressorActive },
                { t: Strings.get("eq_z_mids"), v: equalizerController.zMids,
                  a: equalizerController.zMidsActive },
                { t: Strings.get("eq_z_treble"),
                  v: equalizerController.zTreble,
                  a: equalizerController.zTrebleActive }
            ]
            EqualizerKnob {
                Layout.alignment: Qt.AlignHCenter
                title: modelData.t; value: modelData.v; active: modelData.a
                onValueChangedByUser: (val) =>
                    equalizerController.setEffect1Knob(index, val)
                onToggledByUser: (en) =>
                    equalizerController.toggleEffect1Knob(index, en)
            }
        }
    }
}
