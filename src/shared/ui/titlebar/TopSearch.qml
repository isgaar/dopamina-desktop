/**
 * Location: src/shared/ui/titlebar/TopSearch.qml
 * Function: Header-wide library search field matching the Android pill
 * silhouette, with focus accent, localized placeholder, and recent searches popup.
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
    border.width: (input.activeFocus || recentPopup.opened) ? 1.5 : 1
    border.color: (input.activeFocus || recentPopup.opened) ?
        Theme.primary : Theme.borderDark

    function clear() {
        input.text = ""
        root.edited("")
        recentPopup.close()
    }

    AppIcon {
        id: icon
        name: "search"
        size: 17
        iconColor: (input.activeFocus || recentPopup.opened) ?
            Theme.primary : Theme.textMuted
        anchors.left: parent.left
        anchors.leftMargin: 14
        anchors.verticalCenter: parent.verticalCenter
    }

    Text {
        z: -1
        visible: input.text.length === 0
        text: Strings.get("search_placeholder") || "¿Qué quieres reproducir?"
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
        onClicked: {
            input.forceActiveFocus()
            recentPopup.openBelow(root)
        }
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
        onActiveFocusChanged: {
            if (activeFocus && !recentPopup.opened) {
                recentPopup.openBelow(root)
            }
        }
        onAccepted: {
            if (input.text.trim().length > 0) {
                recentPopup.addRecent({
                    "title": input.text.trim(),
                    "subtitle": "Búsqueda",
                    "type": "song"
                })
            }
            recentPopup.close()
        }
        Keys.onEscapePressed: root.clear()
    }

    RecentSearchesPopup {
        id: recentPopup
        searchField: root
        onTrackSelected: track => {
            input.text = track.title || ""
            root.edited(input.text)
            recentPopup.addRecent(track)
            if (typeof queueController !== "undefined" && queueController && track) {
                queueController.playNow(track)
            }
        }
        onArtistSelected: artist => {
            input.text = artist
            root.edited(artist)
            recentPopup.addRecent({
                "title": artist,
                "subtitle": "Artista",
                "type": "artist",
                "artist": artist
            })
        }
    }
}
