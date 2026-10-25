/**
 * Location: src/features/session/ui/SessionView.qml
 * Function: Session window content switching between the signed-in profile and
 * the Core login form, with transient error feedback.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/feedback"

Rectangle {
    color: Theme.bgDarkest
    radius: Theme.radiusMd

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceLg
        spacing: Theme.spaceMd

        RowLayout {
            Layout.fillWidth: true
            ColumnLayout {
                spacing: 2
                Text {
                    text: Strings.get("session_screen_title")
                    font.family: Theme.fontFamily; font.pixelSize: 18
                    font.bold: true; color: Theme.textLight
                }
                Text {
                    text: Strings.get("session_screen_subtitle")
                    font.family: Theme.fontFamily; font.pixelSize: 11
                    color: Theme.textMuted
                }
            }
            Item { Layout.fillWidth: true }
            LoadingSpinner {
                size: 16; lineWidth: 2
                visible: sessionController.busy
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: errorText.implicitHeight + 16
            visible: sessionController.errorKey.length > 0
            radius: Theme.radiusMd
            color: Theme.withAlpha(Theme.danger, 0.12)
            border.width: 1
            border.color: Theme.withAlpha(Theme.danger, 0.55)

            RowLayout {
                anchors.fill: parent
                anchors.margins: Theme.spaceSm
                spacing: Theme.spaceSm
                AppIcon {
                    name: "circle-alert"; size: 14
                    iconColor: Theme.danger
                }
                Text {
                    id: errorText
                    text: Strings.get(sessionController.errorKey)
                    font.family: Theme.fontFamily; font.pixelSize: 11
                    color: Theme.textLight
                    wrapMode: Text.WordWrap
                    Layout.fillWidth: true
                }
                Rectangle {
                    width: 18; height: 18; radius: 9
                    color: closeArea.containsMouse ?
                        Theme.bgSurface : "transparent"
                    AppIcon {
                        anchors.centerIn: parent
                        name: "close"; size: 10
                        iconColor: Theme.textMuted
                    }
                    MouseArea {
                        id: closeArea
                        anchors.fill: parent
                        hoverEnabled: true
                        cursorShape: Qt.PointingHandCursor
                        onClicked: sessionController.clearError()
                    }
                }
            }
        }

        Loader {
            Layout.fillWidth: true
            Layout.fillHeight: true
            sourceComponent: sessionController.isLoggedIn ?
                profileComponent : loginComponent
        }
    }

    Component { id: profileComponent; SessionProfileCard {} }
    Component { id: loginComponent; SessionLoginForm {} }
}
