/**
 * Location: src/features/settings/ui/ThemePicker.qml
 * Function: Accessible two-column theme selector rendering live XML palette
 * previews for the five Android themes with keyboard support.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

GridLayout {
    id: root
    property string currentTheme: "roseglow"
    signal themeSelected(string themeId)
    columns: 2; columnSpacing: Theme.spaceSm; rowSpacing: Theme.spaceSm

    Repeater {
        model: Colors.availableThemes()
        delegate: Rectangle {
            id: card
            readonly property string themeId: modelData
            readonly property bool selected: root.currentTheme === themeId
            Layout.fillWidth: true; Layout.preferredHeight: 56
            radius: Theme.radiusMd
            color: selected ? Theme.primaryDim :
                (hover.containsMouse ? Theme.bgSurface : "transparent")
            border.width: 1
            border.color: selected ? Theme.primary :
                (activeFocus ? Theme.borderHighlight : Theme.borderDark)
            activeFocusOnTab: true
            Accessible.role: Accessible.RadioButton
            Accessible.name: Strings.get("theme_" + themeId)
            Accessible.checked: selected

            RowLayout {
                anchors.fill: parent; spacing: Theme.spaceSm
                anchors.leftMargin: Theme.spaceSm
                anchors.rightMargin: Theme.spaceSm

                Row {
                    spacing: 3
                    Repeater {
                        model: ["primary", "bg_surface", "text_light"]
                        delegate: Rectangle {
                            width: 12; height: 12; radius: 6
                            color: Colors.paletteColor(card.themeId,
                                                       modelData)
                            border.color: Theme.color("border_subtle")
                            border.width: 1
                        }
                    }
                }

                Text {
                    text: Strings.get("theme_" + card.themeId)
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSm; font.bold: card.selected
                    color: card.selected ? Theme.textLight : Theme.textMuted
                    Layout.fillWidth: true; elide: Text.ElideRight
                }
                AppIcon {
                    name: "check"; size: 13; iconColor: Theme.primary
                    visible: card.selected
                }
            }

            MouseArea {
                id: hover
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onPressed: card.forceActiveFocus()
                onClicked: root.themeSelected(card.themeId)
            }
            Keys.onSpacePressed: root.themeSelected(card.themeId)
            Keys.onReturnPressed: root.themeSelected(card.themeId)
        }
    }
}
