/**
 * Location: src/shared/ui/window/AboutWindow.qml
 * Function: Independent native desktop window displaying Dopamine version,
 * licensing terms, developer credits, and runtime platform architecture.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import ".."

RoundedWindow {
    id: root
    width: 420; height: 320
    minimumWidth: 380; minimumHeight: 280
    maximumWidth: 480; maximumHeight: 360
    title: Strings.get("nav_about") + " " + Theme.appName
    headerTitle: title

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: Theme.spaceLg
        spacing: Theme.spaceMd

        RowLayout {
            spacing: Theme.spaceMd; Layout.alignment: Qt.AlignHCenter

            Image {
                source: "qrc:/Dopamine/Core/src/shared/assets/app-icon.png"
                Layout.preferredWidth: 48; Layout.preferredHeight: 48
                fillMode: Image.PreserveAspectFit
            }

            ColumnLayout {
                spacing: 2
                Text {
                    text: Theme.appName; color: Theme.textLight
                    font.bold: true; font.pixelSize: 18
                    font.family: Theme.fontFamily
                }
                Text {
                    text: Strings.get("about_subtitle")
                    color: Theme.accent; font.pixelSize: 12
                    font.family: Theme.fontFamily
                }
                Text {
                    text: Strings.get("about_version_label") +
                          "2.0.0 • Qt 6.11 / C++20"
                    color: Theme.textMuted; font.pixelSize: 11
                    font.family: Theme.fontFamily
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true; Layout.preferredHeight: 1
            color: Theme.borderDark
        }

        ColumnLayout {
            Layout.fillWidth: true; spacing: Theme.spaceSm

            Text {
                text: Strings.get("about_desc_engine")
                color: Theme.textMuted; font.pixelSize: 12
                font.family: Theme.fontFamily
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
            Text {
                text: Strings.get("about_desc_manager")
                color: Theme.textDim; font.pixelSize: 11
                font.family: Theme.fontFamily
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
            Text {
                text: Strings.get("about_copyright")
                color: Theme.textDim; font.pixelSize: 11
                font.family: Theme.fontFamily
                horizontalAlignment: Text.AlignHCenter
                Layout.fillWidth: true
            }
        }

        Item { Layout.fillHeight: true }

        AppButton {
            text: Strings.get("btn_accept")
            Layout.alignment: Qt.AlignHCenter
            onClicked: root.close()
        }
    }

    Shortcut { sequence: "Escape"; onActivated: root.close() }
}
