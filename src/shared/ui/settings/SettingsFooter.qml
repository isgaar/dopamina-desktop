/**
 * Location: src/shared/ui/settings/SettingsFooter.qml
 * Function: Fixed bottom control bar managing save and discard workflows with
 * status badges and primary action alignment based on dirty state.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

Rectangle {
    id: root
    property bool isDirty: false
    signal closeClicked()
    signal discardClicked()
    signal saveClicked()

    Layout.fillWidth: true
    implicitHeight: 52
    color: Theme.bgDark

    Rectangle {
        anchors.top: parent.top
        width: parent.width
        height: 1
        color: Theme.borderDark
    }

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceMd
        anchors.rightMargin: Theme.spaceMd

        StatusPill {
            variant: root.isDirty ? "warn" : "ok"
            text: root.isDirty ? Strings.get("settings_unsaved_changes") :
                                 Strings.get("settings_all_saved")
        }

        Item { Layout.fillWidth: true }

        AppButton {
            visible: !root.isDirty
            text: Strings.get("settings_close")
            variant: "secondary"
            onClicked: root.closeClicked()
        }

        AppButton {
            visible: root.isDirty
            text: Strings.get("settings_discard")
            variant: "secondary"
            onClicked: root.discardClicked()
        }

        AppButton {
            visible: root.isDirty
            text: Strings.get("settings_save")
            variant: "primary"
            onClicked: root.saveClicked()
        }
    }
}
