/**
 * Location: src/shared/ui/titlebar/RecentSearchesPopup.qml
 * Function: Dropdown popup presenting interactive recent searches, artist and
 * track suggestions with cover art and fast clear capabilities.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

Popup {
    id: root
    parent: Overlay.overlay
    width: Math.max(420, searchField ? searchField.width : 400)
    padding: 12
    modal: false
    focus: false
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var searchField: null
    property var recentList: []

    signal trackSelected(var track)
    signal artistSelected(string artist)

    function openBelow(sourceItem) {
        if (!Overlay.overlay || !sourceItem) {
            root.open();
            return;
        }
        var pt = sourceItem.mapToItem(Overlay.overlay, 0, sourceItem.height + 6);
        var targetX = Math.round(pt.x + (sourceItem.width - root.width) / 2);
        var overlayW = Overlay.overlay.width || 1200;
        var overlayH = Overlay.overlay.height || 800;
        root.x = Math.max(12, Math.min(overlayW - root.width - 12, targetX));
        root.y = Math.max(12, Math.min(overlayH - root.height - 12, Math.round(pt.y)));
        loadRecents();
        root.open();
    }

    function clearAll() {
        recentList = [];
        saveRecents();
    }

    function loadRecents() {
        var raw = "";
        try {
            if (typeof settingsController !== "undefined" && settingsController) {
                raw = settingsController.getSetting("Search/recentItems", "");
            }
        } catch (e) {}

        if (raw && raw.length > 0) {
            try {
                recentList = JSON.parse(raw);
                if (recentList.length > 0) return;
            } catch (e) {}
        }

        // Initialize default curated recents matching Android search parity
        recentList = [
            {
                "id": 101,
                "title": "Corazón Espinado",
                "subtitle": "Canción \u2022 Maná",
                "type": "song",
                "artist": "Maná",
                "album": "Sueños Líquidos",
                "coverUrl": "https://i.scdn.co/image/ab67616d0000b273b41d2f62cb52df49f2b8ebc7"
            },
            {
                "id": 102,
                "title": "Oye Mi Amor",
                "subtitle": "Canción \u2022 Maná",
                "type": "song",
                "artist": "Maná",
                "album": "¿Dónde Jugarán Los Niños?",
                "coverUrl": "https://i.scdn.co/image/ab67616d0000b2733d3c8c76023eb1a4b4ee9874"
            },
            {
                "id": 103,
                "title": "En El Muelle de San Blas",
                "subtitle": "Canción \u2022 Maná",
                "type": "song",
                "artist": "Maná",
                "album": "Sueños Líquidos",
                "coverUrl": "https://i.scdn.co/image/ab67616d0000b273b41d2f62cb52df49f2b8ebc7"
            },
            {
                "id": 104,
                "title": "Maná",
                "subtitle": "Artista \u2022 6 canciones",
                "type": "artist",
                "artist": "Maná",
                "album": "",
                "coverUrl": "https://i.scdn.co/image/ab6761610000e5eb56e87a2fc2e9fba4ebba7d02"
            },
            {
                "id": 105,
                "title": "Corazon Espinado (feat. Mana)",
                "subtitle": "Canción \u2022 Santana",
                "type": "song",
                "artist": "Santana",
                "album": "Supernatural",
                "coverUrl": "https://i.scdn.co/image/ab67616d0000b2730cf19236d6c8b939f506e788"
            }
        ];
    }

    function saveRecents() {
        try {
            if (typeof settingsController !== "undefined" && settingsController) {
                settingsController.setSetting("Search/recentItems", JSON.stringify(recentList));
            }
        } catch (e) {}
    }

    function addRecent(entry) {
        if (!entry || !entry.title) return;
        var next = [entry];
        for (var i = 0; i < recentList.length; ++i) {
            if (recentList[i].title !== entry.title && next.length < 8) {
                next.push(recentList[i]);
            }
        }
        recentList = next;
        saveRecents();
    }

    function selectItem(item) {
        if (!item) return;
        if (item.type === "artist") {
            root.artistSelected(item.artist || item.title);
        } else {
            root.trackSelected(item);
        }
    }

    background: Rectangle {
        radius: 14
        color: "#17171C"
        border.color: "#2D2D3E"
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 6

        // Header Row
        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            Layout.topMargin: 2
            Layout.bottomMargin: 4

            Text {
                text: Strings.get("search_recent_title") || "Búsquedas recientes"
                font.family: Theme.fontFamily
                font.pixelSize: 13
                font.bold: true
                color: "#FFFFFF"
            }

            Item { Layout.fillWidth: true }

            Text {
                text: Strings.get("search_recent_clear") || "Borrar"
                font.family: Theme.fontFamily
                font.pixelSize: 12
                font.bold: true
                color: clearHover.containsMouse ? Theme.primary : "#8E8E9E"
                visible: root.recentList.length > 0

                MouseArea {
                    id: clearHover
                    anchors.fill: parent
                    anchors.margins: -4
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.clearAll()
                }
            }
        }

        Item {
            Layout.fillWidth: true
            Layout.preferredHeight: Math.min(320, listCol.implicitHeight)
            clip: true

            Column {
                id: listCol
                width: parent.width
                spacing: 2

                Repeater {
                    model: root.recentList
                    delegate: Rectangle {
                        id: rowDelegate
                        width: listCol.width
                        height: 52
                        radius: 8
                        color: rowHover.containsMouse ? "#242434" : "transparent"

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 6
                            anchors.rightMargin: 8
                            spacing: 12

                            // Thumbnail Cover
                            Rectangle {
                                Layout.preferredWidth: 38
                                Layout.preferredHeight: 38
                                radius: modelData.type === "artist" ? 19 : 6
                                color: "#20202B"
                                border.color: "#2C2C3C"
                                border.width: 1
                                clip: true

                                AppIcon {
                                    anchors.centerIn: parent
                                    name: modelData.type === "artist" ? "user" : "disc"
                                    size: 18
                                    iconColor: Theme.textDim
                                }

                                RoundedImage {
                                    anchors.fill: parent
                                    radius: parent.radius
                                    source: modelData.coverUrl || ""
                                    visible: source.length > 0
                                }
                            }

                            // Title & Subtitle Column
                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2

                                Text {
                                    text: modelData.title || ""
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 13
                                    font.bold: true
                                    color: "#FFFFFF"
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }

                                Text {
                                    text: modelData.subtitle || ""
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 11
                                    color: "#9292A2"
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                            }
                        }

                        MouseArea {
                            id: rowHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                root.close();
                                root.selectItem(modelData);
                            }
                        }
                    }
                }
            }
        }
    }
}
