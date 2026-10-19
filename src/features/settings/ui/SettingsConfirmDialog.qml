/**
 * Location: src/features/settings/ui/SettingsConfirmDialog.qml
 * Function: In-window modal overlay prompting users to save, discard, or
 * continue editing pending configuration modifications upon close attempt.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

Rectangle {
    id: root
    signal saveRequested()
    signal discardRequested()
    signal keepEditingRequested()

    color: Theme.bgDarkest
    opacity: 0.95

    MouseArea {
        anchors.fill: parent
    }

    Rectangle {
        anchors.centerIn: parent
        width: Math.min(parent.width - 40, 420)
        implicitHeight: cardCol.implicitHeight + Theme.spaceLg * 2
        radius: Theme.radiusMd
        color: Theme.bgSurface
        border.color: Theme.borderDark
        border.width: 1

        ColumnLayout {
            id: cardCol
            anchors.fill: parent
            anchors.margins: Theme.spaceLg
            spacing: Theme.spaceMd

            Text {
                text: Strings.get("settings_confirm_title")
                font.family: Theme.fontFamily
                font.pixelSize: 16
                font.bold: true
                color: Theme.textLight
            }

            Text {
                text: Strings.get("settings_confirm_message")
                font.family: Theme.fontFamily
                font.pixelSize: 12
                color: Theme.textMuted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }

            RowLayout {
                Layout.fillWidth: true
                Layout.topMargin: Theme.spaceSm
                spacing: Theme.spaceSm

                AppButton {
                    text: Strings.get("settings_keep_editing")
                    variant: "secondary"
                    onClicked: root.keepEditingRequested()
                }

                Item { Layout.fillWidth: true }

                AppButton {
                    text: Strings.get("settings_discard")
                    variant: "secondary"
                    onClicked: root.discardRequested()
                }

                AppButton {
                    text: Strings.get("settings_save")
                    variant: "primary"
                    onClicked: root.saveRequested()
                }
            }
        }
    }
}
