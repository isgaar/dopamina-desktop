/**
 * Location: src/features/session/ui/components/SessionMenuRow.qml
 * Function: Android-style session action row with icon, optional badge,
 * supporting subtitle, hover surface, and danger variant.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
Rectangle {
    id: root
    property string iconName: ""
    property string title: ""
    property string subtitle: ""
    property string badge: ""
    property bool danger: false
    signal clicked()
    implicitHeight: 44
    radius: Theme.radiusMd
    color: pointer.containsMouse && !root.danger ?
        Theme.bgSurface : "transparent"
    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: Theme.spaceXs; anchors.rightMargin: Theme.spaceSm
        spacing: Theme.spaceSm
        AppIcon {
            name: root.iconName; size: 16
            iconColor: root.danger ? Theme.danger : Theme.textMuted
        }
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 0
            RowLayout {
                spacing: Theme.spaceXs
                Text {
                    text: root.title
                    font.family: Theme.fontFamily
                    font.pixelSize: 12; font.bold: true
                    color: root.danger ? Theme.danger : Theme.textLight
                }
                Rectangle {
                    visible: root.badge.length > 0
                    width: visible ? badgeLabel.width + 8 : 0
                    height: 14; radius: 7
                    color: Theme.warning
                    Text {
                        id: badgeLabel
                        anchors.centerIn: parent
                        text: root.badge
                        font.family: Theme.fontFamily
                        font.pixelSize: 8; font.bold: true
                        color: Theme.bgDarkest
                    }
                }
            }
            Text {
                text: root.subtitle
                visible: text.length > 0
                font.family: Theme.fontFamily
                font.pixelSize: 10
                color: Theme.textMuted
                elide: Text.ElideRight
                Layout.fillWidth: true
            }
        }
        AppIcon {
            visible: !root.danger
            name: "chevron-right"; size: 12
            iconColor: Theme.textDim
        }
    }
    MouseArea {
        id: pointer
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: root.clicked()
    }
}
