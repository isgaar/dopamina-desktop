/**
 * Location: src/features/playback/ui/equalizer/EqualizerHeader.qml
 * Function: Stellio equalizer header card with curved frame, preset ComboBox,
 * custom preset saving, and one-touch flat response reset button.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

Rectangle {
    id: root
    height: 42; radius: Theme.radiusMd; color: Theme.bgSurface
    border.color: Theme.borderDark; border.width: 1

    RowLayout {
        anchors.fill: parent; anchors.margins: 6; spacing: Theme.spaceSm

        AppIcon {
            name: "sliders"; size: 16; iconColor: Theme.color("eq_coral")
            Layout.alignment: Qt.AlignVCenter
        }

        Text {
            text: Strings.get("eq_title"); font.pixelSize: 12
            font.bold: true; font.family: Theme.fontFamily
            color: Theme.textLight; Layout.alignment: Qt.AlignVCenter
        }

        Item { Layout.fillWidth: true }

        ComboBox {
            id: presetCombo
            Layout.preferredWidth: 150; Layout.preferredHeight: 30
            model: equalizerController.presetNames
            currentIndex: {
                var p = equalizerController.currentPreset
                for (var i = 0; i < model.length; ++i) {
                    if (model[i] === p) return i
                }
                return 0
            }
            displayText: equalizerController.currentPreset
            onActivated: equalizerController.loadPreset(currentText)

            contentItem: TextInput {
                readOnly: true; selectByMouse: false
                text: presetCombo.displayText
                color: Theme.textLight; font.pixelSize: 12
                font.bold: true; font.family: Theme.fontFamily
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }

            background: Rectangle {
                color: Theme.bgCard; radius: Theme.radiusSm
                border.color: Theme.borderDark; border.width: 1
            }
        }

        AppButton {
            Layout.preferredWidth: 30; Layout.preferredHeight: 30
            iconName: "bookmark"; variant: "secondary"
            onClicked: {
                var p = equalizerController.currentPreset
                equalizerController.savePreset("Preset " +
                    (equalizerController.presetNames.length + 1))
            }
        }

        AppButton {
            Layout.preferredWidth: 30; Layout.preferredHeight: 30
            iconName: "rotate-ccw"; variant: "secondary"
            onClicked: equalizerController.reset()
        }
    }
}
