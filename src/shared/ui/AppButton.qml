/**
 * Location: src/shared/ui/AppButton.qml
 * Function: Reusable styled button component honoring the native dark teal
 * theme tokens with hover state transitions and integrated Lucide vector icons.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts

Rectangle {
    id: root
    property string text: ""
    property string iconName: ""
    property string variant: "primary"
    signal clicked()

    implicitHeight: 34
    implicitWidth: Math.max(92, contentRow.implicitWidth + Theme.spaceMd * 2)
    radius: Theme.radiusMd
    opacity: root.enabled ? (mouseArea.pressed ? 0.85 : 1.0) : 0.45

    color: {
        if (root.variant === "secondary")
            return mouseArea.containsMouse ? Theme.bgCard : Theme.bgSurface
        if (root.variant === "danger")
            return mouseArea.containsMouse ? Qt.darker(Theme.danger, 1.15)
                                           : Theme.danger
        if (root.variant === "ghost")
            return mouseArea.containsMouse ? Theme.bgSurface : "transparent"
        return mouseArea.containsMouse ? Theme.primaryHover : Theme.primary
    }

    border.color: {
        if (root.variant === "secondary")
            return mouseArea.containsMouse ? Theme.accent : Theme.borderDark
        if (root.variant === "ghost")
            return mouseArea.containsMouse ? Theme.borderDark : "transparent"
        if (root.variant === "danger") return Theme.danger
        return Theme.accent
    }
    border.width: 1

    Behavior on color { ColorAnimation { duration: 120 } }
    Behavior on border.color { ColorAnimation { duration: 120 } }

    RowLayout {
        id: contentRow
        anchors.centerIn: parent
        spacing: Theme.spaceSm

        AppIcon {
            visible: root.iconName !== ""
            name: root.iconName
            size: 13
            iconColor: {
                if (root.variant === "ghost" && !mouseArea.containsMouse)
                    return Theme.textMuted
                return (root.variant === "primary" ||
                        root.variant === "danger")
                    ? Theme.bgDarkest : Theme.textLight
            }
        }

        Text {
            visible: root.text !== ""
            text: root.text
            color: {
                if (root.variant === "ghost" && !mouseArea.containsMouse)
                    return Theme.textMuted
                return (root.variant === "primary" ||
                        root.variant === "danger")
                    ? Theme.bgDarkest : Theme.textLight
            }
            font.bold: true
            font.pixelSize: 12
            font.family: Theme.fontFamily
        }
    }

    MouseArea {
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: root.enabled
        enabled: root.enabled
        cursorShape: root.enabled ? Qt.PointingHandCursor : Qt.ArrowCursor
        onClicked: if (root.enabled) root.clicked()
    }
}
