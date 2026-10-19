/**
 * Location: src/shared/ui/state/BannerNotification.qml
 * Function: Floating top banner alert presenting localized feedback,
 * actionable Undo/Retry triggers, and animated auto-dismiss countdown timer.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

Rectangle {
    id: root

    property string message: ""
    property string actionText: Strings.get("btn_undo")
    property bool hasAction: true
    property int timeoutMs: 8000
    signal actionTriggered()
    signal dismissed()

    implicitHeight: 40
    color: Theme.bgSurface
    radius: Theme.radiusMd
    border.color: Theme.borderDark
    border.width: 1

    RowLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceSm
        spacing: Theme.spaceMd

        AppIcon {
            name: "info"
            size: 16
            iconColor: Theme.accent
        }

        Text {
            Layout.fillWidth: true
            text: root.message
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            color: Theme.textLight
            elide: Text.ElideRight
        }

        AppButton {
            visible: root.hasAction
            text: root.actionText
            onClicked: {
                dismissTimer.stop();
                root.actionTriggered();
                root.dismissed();
            }
        }
    }

    Timer {
        id: dismissTimer
        interval: root.timeoutMs
        running: true
        repeat: false
        onTriggered: root.dismissed()
    }
}
