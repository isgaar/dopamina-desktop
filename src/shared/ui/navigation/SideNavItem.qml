/**
 * Location: src/shared/ui/navigation/SideNavItem.qml
 * Function: Navigation row with full capsule pill, Lucide glyph, label,
 * contrast-aware text, and a tooltip when the rail is collapsed.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import ".."

Rectangle {
    id: root
    property string iconName: "music"
    property string label: ""
    property bool active: false
    property bool collapsed: false
    signal clicked()

    readonly property real primaryLum:
        0.299 * Theme.primary.r + 0.587 * Theme.primary.g +
        0.114 * Theme.primary.b
    readonly property color activeFg:
        primaryLum > 0.6 ? "#121212" : "#FFFFFF"

    height: 42
    radius: Theme.radiusFull
    color: root.active ?
        Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.16) :
        (hover.containsMouse ? Theme.bgSurface : "transparent")
    border.width: root.active ? 1 : 0
    border.color: root.active ?
        Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.40) : "transparent"
    Behavior on color { ColorAnimation { duration: 160 } }
    Behavior on border.color { ColorAnimation { duration: 160 } }

    AppIcon {
        id: glyph
        name: root.iconName
        size: 18
        width: size; height: size
        x: root.collapsed ? (root.width - width) / 2 : Theme.spaceMd
        anchors.verticalCenter: parent.verticalCenter
        iconColor: root.active ? Theme.primary : (hover.containsMouse ? "#FFFFFF" : Theme.textMuted)
        Behavior on iconColor { ColorAnimation { duration: 160 } }
    }

    Text {
        visible: !root.collapsed
        x: glyph.x + glyph.width + Theme.spaceSm
        width: root.width - x - Theme.spaceSm
        anchors.verticalCenter: parent.verticalCenter
        text: root.label
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontBase
        font.bold: root.active
        color: root.active ? Theme.primary : (hover.containsMouse ? "#FFFFFF" : Theme.textLight)
        elide: Text.ElideRight
        Behavior on color { ColorAnimation { duration: 160 } }
    }

    MouseArea {
        id: hover
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }

    ToolTip.visible: root.collapsed && hover.containsMouse
    ToolTip.text: root.label
    ToolTip.delay: 350
}
