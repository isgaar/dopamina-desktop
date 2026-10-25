/**
 * Location: src/shared/ui/titlebar/TopSearch.qml
 * Function: Header-wide library search field matching the Android pill
 * silhouette, with focus accent and a localized placeholder.
 * All Rights Reserved isgaar
 */
import QtQuick
import "root:/src/shared"
import "root:/src/shared/ui"

Rectangle {
    id: root
    property alias text: input.text
    signal edited(string text)

    height: 36
    radius: height / 2
    color: Theme.withAlpha(Theme.bgSurface, 0.55)
    border.width: 1
    border.color: input.activeFocus ?
        Theme.withAlpha(Theme.primary, 0.65) : Theme.borderDark

    function clear() {
        input.text = ""
        root.edited("")
    }

    AppIcon {
        id: icon
        name: "search"
        size: 17
        iconColor: input.activeFocus ? Theme.primaryHover : Theme.textMuted
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
    }

    Text {
        z: -1
        visible: input.text.length === 0
        text: Strings.get("search_placeholder")
        color: Theme.textDim
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontBase
        anchors.left: input.left
        anchors.right: input.right
        anchors.verticalCenter: parent.verticalCenter
        elide: Text.ElideRight
    }

    MouseArea {
        anchors.fill: parent
        onClicked: input.forceActiveFocus()
    }

    TextInput {
        id: input
        anchors.left: icon.right
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 14
        anchors.verticalCenter: parent.verticalCenter
        color: Theme.textLight
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontBase
        selectByMouse: true
        clip: true
        onTextChanged: root.edited(text)
        Keys.onEscapePressed: root.clear()
    }
}
