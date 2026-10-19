/**
 * Location: src/shared/ui/settings/SettingsField.qml
 * Function: Standardized settings input field with top label, monospace font
 * support for URIs, 32px height constraint, and contextual help hints.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"

ColumnLayout {
    id: root
    property string label: ""
    property alias text: input.text
    property string placeholderText: ""
    property string helpText: ""
    property bool monospace: false
    property bool readOnly: false
    property alias echoMode: input.echoMode
    signal textEdited(string value)

    Layout.fillWidth: true
    spacing: Theme.spaceXs

    Text {
        text: root.label
        font.family: Theme.fontFamily
        font.pixelSize: 12
        font.bold: true
        color: Theme.textLight
        visible: root.label.length > 0
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 32
        radius: Theme.radiusMd
        color: root.readOnly ? Theme.bgDark : Theme.bgSurface
        border.color: input.activeFocus ? Theme.accent : Theme.borderDark
        border.width: 1

        TextInput {
            id: input
            anchors.fill: parent
            anchors.leftMargin: Theme.spaceSm
            anchors.rightMargin: Theme.spaceSm
            verticalAlignment: TextInput.AlignVCenter
            font.family: root.monospace ? "monospace" : Theme.fontFamily
            font.pixelSize: 12
            color: root.readOnly ? Theme.textMuted : Theme.textLight
            readOnly: root.readOnly
            selectByMouse: true
            clip: true
            onTextEdited: root.textEdited(input.text)

            Text {
                anchors.fill: parent
                verticalAlignment: Text.AlignVCenter
                text: root.placeholderText
                font.family: input.font.family
                font.pixelSize: 12
                color: Theme.textDim
                visible: !input.text && !input.activeFocus
            }
        }
    }

    Text {
        text: root.helpText
        font.family: Theme.fontFamily
        font.pixelSize: 11
        color: Theme.textDim
        wrapMode: Text.WordWrap
        Layout.fillWidth: true
        visible: root.helpText.length > 0
    }
}
