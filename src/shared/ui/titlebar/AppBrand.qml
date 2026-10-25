/**
 * Location: src/shared/ui/titlebar/AppBrand.qml
 * Function: Dopamine launcher mark and wordmark paired for the desktop
 * application header.
 * All Rights Reserved isgaar
 */
import QtQuick
import "root:/src/shared"

Item {
    id: root
    implicitWidth: row.implicitWidth
    implicitHeight: row.implicitHeight

    Row {
        id: row
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.spaceSm

        Image {
            source: "qrc:/Dopamine/Core/src/shared/assets/app-icon.png"
            width: 32; height: 32
            fillMode: Image.PreserveAspectFit
        }

        Text {
            text: Theme.appName
            font.family: Theme.fontFamily
            font.pixelSize: 19
            font.bold: true
            color: Theme.textLight
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
