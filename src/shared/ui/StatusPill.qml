/**
 * Location: src/shared/ui/StatusPill.qml
 * Function: Compact state indicator pill rendering semantic color tokens,
 * descriptive labels, and mandatory Lucide vector iconography across views.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

Rectangle {
    id: root
    property string variant: "neutral"
    property string text: ""
    property string iconName: ""

    implicitHeight: 24
    implicitWidth: pillRow.implicitWidth + Theme.spaceSm * 2
    radius: Theme.radiusFull

    color: {
        if (root.variant === "ok") return Theme.color("success_badge")
        if (root.variant === "error") return Theme.color("danger_badge")
        if (root.variant === "warn") return Theme.color("primary_dim")
        return Theme.bgSurface
    }

    border.color: {
        if (root.variant === "ok") return Theme.success
        if (root.variant === "error") return Theme.danger
        if (root.variant === "warn") return Theme.warning
        return Theme.borderDark
    }
    border.width: 1

    RowLayout {
        id: pillRow
        anchors.centerIn: parent
        spacing: Theme.spaceXs

        AppIcon {
            name: {
                if (root.iconName.length > 0) return root.iconName
                if (root.variant === "ok") return "check"
                if (root.variant === "error") return "x"
                if (root.variant === "warn") return "triangle-alert"
                return "info"
            }
            size: 11
            iconColor: {
                if (root.variant === "ok") return Theme.success
                if (root.variant === "error") return Theme.danger
                if (root.variant === "warn") return Theme.warning
                return Theme.textMuted
            }
        }

        Text {
            text: root.text
            font.family: Theme.fontFamily
            font.pixelSize: 11
            font.bold: true
            color: {
                if (root.variant === "ok") return Theme.success
                if (root.variant === "error") return Theme.danger
                if (root.variant === "warn") return Theme.warning
                return Theme.textMuted
            }
            visible: root.text.length > 0
        }
    }
}
