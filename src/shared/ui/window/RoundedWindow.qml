/**
 * Location: src/shared/ui/window/RoundedWindow.qml
 * Function: Generalized base desktop window implementing 4-corner rounded
 * chrome, frameless system dragging, custom title controls, and auto-setup.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Window

Window {
    id: root
    color: "transparent"
    flags: Platform.useCustomTitleBar ?
        (Qt.Window | Qt.FramelessWindowHint) : Qt.Window

    property string headerTitle: root.title
    property bool showCloseButton: true
    property bool isFloating: visibility !== Window.Maximized
    default property alias windowContent: contentHost.data

    function showWindow() {
        root.show(); root.raise(); root.requestActivate();
    }

    Component.onCompleted: Platform.setupWindow(root)

    Rectangle {
        id: chromeRoot
        anchors.fill: parent
        color: Theme.bgDarkest
        radius: root.isFloating ? Theme.radiusMd : 0
        border.color: root.isFloating ? Theme.borderDark : "transparent"
        border.width: 1
        clip: true

        Rectangle {
            id: headerBar
            anchors.top: parent.top; anchors.left: parent.left
            anchors.right: parent.right; height: 34
            color: "transparent"

            MouseArea {
                anchors.fill: parent
                onPressed: root.startSystemMove()
            }

            RowLayout {
                anchors.fill: parent
                anchors.leftMargin: Theme.spaceMd
                anchors.rightMargin: Theme.spaceSm
                spacing: Theme.spaceSm

                Image {
                    source: "qrc:/Dopamine/Core/src/shared/assets/app-icon.png"
                    Layout.preferredWidth: 16; Layout.preferredHeight: 16
                    Layout.alignment: Qt.AlignVCenter
                    fillMode: Image.PreserveAspectFit
                }

                Text {
                    text: root.headerTitle; color: Theme.textMuted
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSm
                    Layout.fillWidth: true
                    Layout.alignment: Qt.AlignVCenter
                    elide: Text.ElideRight
                }

                Rectangle {
                    Layout.preferredWidth: 26; Layout.preferredHeight: 26
                    Layout.alignment: Qt.AlignVCenter
                    radius: Theme.radiusSm
                    color: closeBtnMouse.containsMouse ?
                        Theme.danger : "transparent"
                    visible: root.showCloseButton

                    Text {
                        anchors.centerIn: parent; text: "✕"
                        color: closeBtnMouse.containsMouse ?
                            Theme.bgDarkest : Theme.textMuted
                        font.pixelSize: 11; font.family: Theme.fontFamily
                    }

                    MouseArea {
                        id: closeBtnMouse; anchors.fill: parent
                        hoverEnabled: true; cursorShape: Qt.PointingHandCursor
                        onClicked: root.close()
                    }
                }
            }

            Rectangle {
                anchors.bottom: parent.bottom; width: parent.width; height: 1
                color: Theme.borderDark
            }
        }

        Item {
            id: contentHost
            anchors.top: headerBar.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
        }
    }
}
