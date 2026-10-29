/**
 * Location: src/features/library/ui/ContentTabs.qml
 * Function: Rounded pill tab strip for switching library sections with
 * dynamic active state and subtle inactive surfaces matching web console.
 * All Rights Reserved isgaar
 */
import QtQuick

Item {
    id: root
    height: 40

    property string current: "tracks"
    signal selected(string mode)

    readonly property real primaryLum:
        0.299 * Theme.primary.r + 0.587 * Theme.primary.g +
        0.114 * Theme.primary.b
    readonly property color activeFg:
        primaryLum > 0.6 ? "#121212" : "#FFFFFF"

    readonly property var tabs: [
        { mode: "listen", label: Strings.get("nav_listen") || "Escuchar" },
        { mode: "tracks", label: Strings.get("nav_tracks") },
        { mode: "albums", label: Strings.get("nav_albums") },
        { mode: "artists", label: Strings.get("nav_artists") },
        { mode: "playlists", label: Strings.get("nav_playlists") },
        { mode: "favorites", label: Strings.get("nav_favorites") },
        { mode: "queue", label: Strings.get("nav_queue") }
    ]

    Row {
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        spacing: Theme.spaceSm

        Repeater {
            model: root.tabs
            delegate: Rectangle {
                id: pill
                readonly property bool active:
                    modelData.mode === "tracks" ?
                    (root.current === "tracks" ||
                     root.current === "folders" ||
                     false) :
                    (modelData.mode === "listen" ?
                        (root.current === "listen" || root.current === "principal") :
                    (root.current === modelData.mode)
                width: label.implicitWidth + 28
                height: 32
                radius: Theme.radiusFull
                color: active ?
                    Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.16) :
                    (hover.containsMouse ? Theme.bgSurface :
                     Theme.withAlpha(Theme.bgSurface, 0.45))
                border.width: 1
                border.color: active ?
                    Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.45) :
                    Theme.withAlpha(Theme.borderDark, 0.5)
                Behavior on color { ColorAnimation { duration: 160 } }
                Behavior on border.color { ColorAnimation { duration: 160 } }

                Text {
                    id: label
                    anchors.centerIn: parent
                    text: modelData.label
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                    font.bold: pill.active
                    color: pill.active ? Theme.primary : (hover.containsMouse ? "#FFFFFF" : Theme.textMuted)
                    Behavior on color { ColorAnimation { duration: 160 } }
                }

                MouseArea {
                    id: hover
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.selected(modelData.mode)
                }
            }
        }
    }
}
