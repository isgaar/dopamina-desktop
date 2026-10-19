/**
 * Location: src/shared/ui/settings/SettingsSwitchRow.qml
 * Function: Standardized binary toggle switch row presenting feature title,
 * descriptive explanation, and styled state indicator with Theme tokens.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"

RowLayout {
    id: root
    property string title: ""
    property alias label: root.title
    property string subtitle: ""
    property bool checked: false
    signal toggled(bool isChecked)

    Layout.fillWidth: true
    spacing: Theme.spaceMd

    ColumnLayout {
        Layout.fillWidth: true
        spacing: 2

        Text {
            text: root.title
            font.family: Theme.fontFamily
            font.pixelSize: 12
            font.bold: true
            color: Theme.textLight
            Layout.fillWidth: true
        }

        Text {
            text: root.subtitle
            font.family: Theme.fontFamily
            font.pixelSize: 11
            color: Theme.textDim
            wrapMode: Text.WordWrap
            Layout.fillWidth: true
            visible: root.subtitle.length > 0
        }
    }

    Switch {
        id: switchControl
        checked: root.checked
        onToggled: {
            root.checked = checked
            root.toggled(checked)
        }

        indicator: Rectangle {
            implicitWidth: 38
            implicitHeight: 20
            radius: Theme.radiusFull
            color: switchControl.checked ? Theme.primary : Theme.bgSurface
            border.color: switchControl.checked ? Theme.accent :
                          Theme.borderDark
            border.width: 1

            Rectangle {
                x: switchControl.checked ? parent.width - width - 2 : 2
                anchors.verticalCenter: parent.verticalCenter
                width: 16
                height: 16
                radius: Theme.radiusFull
                color: switchControl.checked ? Theme.bgDarkest :
                       Theme.textMuted
                Behavior on x { NumberAnimation { duration: 100 } }
            }
        }
    }
}
