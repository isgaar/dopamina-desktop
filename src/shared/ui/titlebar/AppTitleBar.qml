/**
 * Location: src/shared/ui/titlebar/AppTitleBar.qml
 * Function: Master custom title bar assembling integrated menu bar, center
 * window title, server connection status pill, and window controls.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import QtQuick.Window
import "../settings"

Rectangle {
    id: root
    height: 34
    color: "transparent"

    property var window: null
    property bool isFirstRun: false
    property string activeView: "radio"
    signal viewChangeRequested(string view)
    signal openAboutRequested()
    signal openSettingsRequested()
    signal openEqualizerRequested()

    Rectangle {
        anchors.bottom: parent.bottom; width: parent.width; height: 1
        color: Theme.borderDark
    }

    MouseArea {
        anchors.fill: parent
        onPressed: if (root.window) root.window.startSystemMove()
        onDoubleClicked: if (root.window && !root.isFirstRun) {
            root.window.visibility === Window.Maximized ?
                root.window.showNormal() : root.window.showMaximized()
        }
    }

    RowLayout {
        anchors.fill: parent; spacing: 0
        Item { width: Theme.spaceSm }

        Image {
            source: "qrc:/Dopamine/Core/src/shared/assets/app-icon.png"
            Layout.preferredWidth: 16; Layout.preferredHeight: 16
            Layout.alignment: Qt.AlignVCenter
            fillMode: Image.PreserveAspectFit
        }
        Item { width: Theme.spaceSm }

        TitleMenuBar {
            id: titleMenuBar
            Layout.preferredHeight: 26; Layout.alignment: Qt.AlignVCenter
            visible: Platform.useCustomTitleBar &&
                !Platform.useSystemMenuExport && !root.isFirstRun
            onOpenAboutRequested: root.openAboutRequested()
            onOpenSettingsRequested: root.openSettingsRequested()
            onOpenEqualizerRequested: root.openEqualizerRequested()
        }

        Item { Layout.fillWidth: true }

        Text {
            text: Theme.appName
            font.family: Theme.fontFamily
            font.pixelSize: Theme.fontSm
            font.bold: true
            color: Theme.textMuted
            Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
        }

        Item { Layout.fillWidth: true }

        StatusPill {
            Layout.alignment: Qt.AlignVCenter
            visible: !root.isFirstRun
            variant: {
                var s = settingsController.connectionStatus
                return s === "ok" ? "ok" :
                    (s === "connecting" ? "warn" : "error")
            }
            text: {
                var s = settingsController.connectionStatus
                if (s === "ok") {
                    return Strings.get("status_connected_to") + " " +
                        (config.targetHost.length > 0 ?
                            config.targetHost : "127.0.0.1")
                }
                return s === "connecting" ?
                    Strings.get("settings_status_checking") :
                    Strings.get("settings_status_disconnected")
            }
        }

        Item { width: Theme.spaceSm }

        WindowControls {
            Layout.alignment: Qt.AlignRight | Qt.AlignVCenter
            visible: Platform.useCustomTitleBar
            window: root.window; canMaximize: !root.isFirstRun
        }
    }
}
