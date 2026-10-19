/**
 * Location: src/shared/ui/settings/SettingsPane.qml
 * Function: Reusable container pane providing standardized category headers,
 * contextual descriptions, and scrollable hosts for preference views.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"

Flickable {
    id: root
    property string title: ""
    property string description: ""
    default property alias content: paneHost.data

    contentWidth: width
    contentHeight: mainCol.implicitHeight + Theme.spaceLg * 2
    clip: true
    boundsBehavior: Flickable.StopAtBounds

    ScrollBar.vertical: ScrollBar {
        policy: root.contentHeight > root.height ?
            ScrollBar.AsNeeded : ScrollBar.AlwaysOff
    }

    ColumnLayout {
        id: mainCol
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: Theme.spaceLg
        spacing: Theme.spaceMd

        ColumnLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceXs

            Text {
                text: root.title
                font.family: Theme.fontFamily
                font.pixelSize: 18
                font.bold: true
                color: Theme.textLight
                visible: root.title.length > 0
            }

            Text {
                text: root.description
                font.family: Theme.fontFamily
                font.pixelSize: 12
                color: Theme.textMuted
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
                visible: root.description.length > 0
            }
        }

        Rectangle {
            Layout.fillWidth: true
            height: 1
            color: Theme.borderDark
            visible: root.title.length > 0
        }

        ColumnLayout {
            id: paneHost
            Layout.fillWidth: true
            spacing: Theme.spaceMd
        }
    }
}
