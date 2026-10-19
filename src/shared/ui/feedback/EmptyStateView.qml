/**
 * Location: src/shared/ui/feedback/EmptyStateView.qml
 * Function: Reusable empty state view presenting localized placeholder copy,
 * vector icon glyph, optional action button, and loading spinner animation.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

Item {
    id: root
    property string iconName: "disc"
    property string title: Strings.get("empty_section_title")
    property string description: Strings.get("empty_section_desc")
    property string actionText: Strings.get("empty_btn_retry")
    property bool showAction: false
    property bool isLoading: false
    signal actionTriggered()

    ColumnLayout {
        anchors.centerIn: parent
        spacing: Theme.spaceMd
        width: Math.min(parent.width - Theme.spaceXl * 2, 420)

        LoadingSpinner {
            Layout.alignment: Qt.AlignHCenter
            visible: root.isLoading
            size: 40
        }

        AppIcon {
            Layout.alignment: Qt.AlignHCenter
            visible: !root.isLoading
            name: root.iconName
            size: 48
            iconColor: Theme.textDim
        }

        Text {
            Layout.fillWidth: true
            text: root.title
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontLg
            font.bold: true
            color: Theme.textLight
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            text: root.description
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            color: Theme.textMuted
            horizontalAlignment: Text.AlignHCenter
            wrapMode: Text.WordWrap
            visible: root.description.length > 0
        }

        AppButton {
            Layout.alignment: Qt.AlignHCenter
            visible: root.showAction && !root.isLoading
            text: root.actionText
            variant: "secondary"
            onClicked: root.actionTriggered()
        }
    }
}
