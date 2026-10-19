/**
 * Location: src/shared/ui/settings/SettingsNav.qml
 * Function: Reusable responsive navigation bar and rail for settings views
 * featuring Lucide icon badges and soft emerald selection states.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

Rectangle {
    id: root
    property var categories: []
    property int currentIndex: 0
    property bool horizontal: false
    signal categorySelected(int index)
    color: "transparent"
    implicitWidth: horizontal ? parent.width : 190
    implicitHeight: horizontal ? 44 : parent.height

    ListView {
        id: navList
        anchors.fill: parent
        anchors.margins: Theme.spaceSm
        spacing: Theme.spaceXs
        orientation: root.horizontal ? ListView.Horizontal : ListView.Vertical
        model: root.categories
        interactive: false

        delegate: Rectangle {
            id: navItem
            readonly property bool isSelected: index === root.currentIndex
            readonly property bool isHovered: itemMouse.containsMouse
            width: root.horizontal ? 130 : navList.width
            height: 36
            radius: Theme.radiusSm
            color: isSelected ? Theme.accentLight :
                   (isHovered ? Theme.bgSurface : "transparent")

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.spaceSm
                spacing: Theme.spaceSm

                AppIcon {
                    name: modelData.icon || "sliders"
                    size: 14
                    iconColor: navItem.isSelected ? Theme.primary :
                               (navItem.isHovered ? Theme.textLight :
                                Theme.textMuted)
                }

                Text {
                    text: modelData.title || ""
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                    font.bold: navItem.isSelected
                    color: navItem.isSelected ? Theme.primary :
                           (navItem.isHovered ? Theme.textLight :
                            Theme.textMuted)
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            MouseArea {
                id: itemMouse
                anchors.fill: parent
                hoverEnabled: true
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    root.currentIndex = index
                    root.categorySelected(index)
                }
            }
        }
    }
}
