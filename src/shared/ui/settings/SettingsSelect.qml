/**
 * Location: src/shared/ui/settings/SettingsSelect.qml
 * Function: Standardized dropdown picker component with top label, Theme token
 * styling, 32px height constraint, and contextual help descriptions.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

ColumnLayout {
    id: root
    property string label: ""
    property string helpText: ""
    property var options: []
    property string currentValue: ""
    property alias model: combo.model
    property alias currentIndex: combo.currentIndex
    property alias currentText: combo.currentText
    signal activated(int index)
    signal selected(string val)

    Layout.fillWidth: true; spacing: Theme.spaceXs

    Text {
        text: root.label; font.family: Theme.fontFamily
        font.pixelSize: 12; font.bold: true; color: Theme.textLight
        visible: root.label.length > 0
    }

    ComboBox {
        id: combo
        Layout.fillWidth: true; Layout.preferredHeight: 32
        model: root.options.length > 0 ? root.options : []
        textRole: "label"; valueRole: "value"
        currentIndex: {
            for (var i = 0; i < root.options.length; ++i) {
                if (root.options[i].value === root.currentValue) return i;
            }
            return 0;
        }
        onActivated: idx => {
            root.activated(idx)
            if (root.options[idx] && root.options[idx].value) {
                root.currentValue = root.options[idx].value
                root.selected(root.currentValue)
            }
        }

        background: Rectangle {
            radius: Theme.radiusSm; color: Theme.bgSurface
            border.color: combo.activeFocus ? Theme.accent : Theme.borderDark
            border.width: 1
        }

        contentItem: TextInput {
            readOnly: true; selectByMouse: false
            leftPadding: Theme.spaceSm; text: combo.displayText
            font.family: Theme.fontFamily; font.pixelSize: 12
            color: Theme.textLight; verticalAlignment: Text.AlignVCenter
        }

        indicator: AppIcon {
            anchors.right: parent.right; anchors.rightMargin: Theme.spaceSm
            anchors.verticalCenter: parent.verticalCenter
            name: "chevron-down"; size: 14; iconColor: Theme.textMuted
        }
    }

    Text {
        text: root.helpText; font.family: Theme.fontFamily
        font.pixelSize: 11; color: Theme.textDim; wrapMode: Text.WordWrap
        Layout.fillWidth: true; visible: root.helpText.length > 0
    }
}
