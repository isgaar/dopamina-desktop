/**
 * Location: src/features/playback/ui/equalizer/EqualizerBandsTab.qml
 * Function: Stellio Bands tab featuring curve visualizer, preamp fader,
 * limiter toggle, and 12-band frequency tracks with fluid rounded sliders.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

ColumnLayout {
    id: root; spacing: Theme.spaceSm

    RowLayout {
        Layout.fillWidth: true; spacing: Theme.spaceSm
        EqualizerCurve { Layout.fillWidth: true; Layout.preferredHeight: 74 }
        ColumnLayout {
            Layout.preferredWidth: 100; spacing: 6
            AppButton {
                Layout.fillWidth: true; Layout.preferredHeight: 32
                text: Strings.get("eq_reset"); iconName: "rotate-ccw"
                variant: "secondary"
                onClicked: equalizerController.resetBands()
            }
            AppButton {
                Layout.fillWidth: true; Layout.preferredHeight: 32
                text: Strings.get("eq_limit"); iconName: "power"
                variant: equalizerController.limiter ? "primary" : "secondary"
                onClicked: equalizerController.setLimiter(
                    !equalizerController.limiter)
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true; Layout.preferredHeight: 38
        radius: Theme.radiusSm; color: Theme.bgCard
        border.color: Theme.borderDark; border.width: 1

        RowLayout {
            anchors.fill: parent; anchors.margins: Theme.spaceSm
            spacing: Theme.spaceSm

            Text {
                text: Strings.get("eq_preamp"); font.pixelSize: 11
                font.bold: true; color: Theme.textMuted
                Layout.preferredWidth: 64
            }
            Slider {
                id: preampSlider; Layout.fillWidth: true
                from: -12.0; to: 12.0; stepSize: 0.5
                Binding on value {
                    when: !preampSlider.pressed
                    value: equalizerController.preamp
                }
                onMoved: equalizerController.setPreamp(value)
            }
            Text {
                text: (preampSlider.value > 0 ? "+" : "") +
                    preampSlider.value.toFixed(1) + " dB"
                font.pixelSize: 11; font.bold: true
                color: Math.round(preampSlider.value) !== 0 ?
                    Theme.color("eq_coral") : Theme.textDim
                Layout.preferredWidth: 48; horizontalAlignment: Text.AlignRight
            }
        }
    }

    Rectangle {
        Layout.fillWidth: true; Layout.preferredHeight: 12 * 28 + 12
        radius: Theme.radiusMd; color: Theme.bgCard
        border.color: Theme.borderDark; border.width: 1

        ColumnLayout {
            anchors.fill: parent; anchors.margins: 6; spacing: 2

            Repeater {
                model: equalizerController.bandLabels.length
                RowLayout {
                    Layout.fillWidth: true; Layout.preferredHeight: 26
                    spacing: 6
                    Text {
                        text: equalizerController.bandLabels[index]
                        font.pixelSize: 11; color: Theme.textMuted
                        Layout.preferredWidth: 54
                    }
                    Slider {
                        id: bSlider; Layout.fillWidth: true
                        from: -12.0; to: 12.0; stepSize: 0.5
                        Binding on value {
                            when: !bSlider.pressed
                            value: (equalizerController.bandGains &&
                                    index <
                                    equalizerController.bandGains.length) ?
                                equalizerController.bandGains[index] : 0.0
                        }
                        onMoved: equalizerController.setBandGain(index, value)
                    }
                    Text {
                        text: (bSlider.value > 0 ? "+" : "") +
                              Math.round(bSlider.value) + " dB"
                        font.pixelSize: 11
                        color: Math.round(bSlider.value) !== 0 ?
                            Theme.color("eq_coral") : Theme.textDim
                        Layout.preferredWidth: 42
                        horizontalAlignment: Text.AlignRight
                    }
                }
            }
        }
    }
}
