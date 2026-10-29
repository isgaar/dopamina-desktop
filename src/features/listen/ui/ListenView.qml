/**
 * Location: src/features/listen/ui/ListenView.qml
 * Function: Modern Apple-inspired principal discovery home view presenting
 * quick picks, latest dynamic mixes, newly added server content, and synced playlists.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import QtQuick.Controls
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/feedback"

Item {
    id: root
    clip: true

    property var libraryCtrl: null
    property var playlistCtrl: null
    property var playbackCtrl: null
    property var queueCtrl: null
    property var sessionCtrl: null

    readonly property bool isOnline: playlistCtrl ? playlistCtrl.isServerOnline : true

    Component.onCompleted: {
        if (libraryCtrl) {
            libraryCtrl.loadTracks(1);
            libraryCtrl.loadRecentTracks();
        }
        if (playlistCtrl) {
            playlistCtrl.loadPlaylists();
        }
    }

    Flickable {
        id: flick
        anchors.fill: parent
        contentWidth: width
        contentHeight: contentCol.implicitHeight + Theme.spaceXl * 2
        clip: true
        boundsBehavior: Flickable.StopAtBounds

        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AsNeeded
            active: flick.moving || flick.fllicking
        }

        ColumnLayout {
            id: contentCol
            width: parent.width
            spacing: Theme.spaceLg

            // ==================== OFFLINE NOTICE (IF OFFLINE) ====================
            Rectangle {
                visible: !root.isOnline
                Layout.fillWidth: true
                Layout.preferredHeight: 36
                radius: Theme.radiusMd
                color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.15)
                border.color: Qt.rgba(Theme.accent.r, Theme.accent.g, Theme.accent.b, 0.4)
                border.width: 1

                RowLayout {
                    anchors.centerIn: parent
                    spacing: Theme.spaceSm
                    AppIcon { name: "wifi-off"; size: 14; iconColor: Theme.accent }
                    Text {
                        text: Strings.get("state_offline_notice") ||
                              "Modo sin conexión: mostrando solo playlists y contenido descargado"
                        font.family: Theme.fontFamily
                        font.pixelSize: 11
                        font.bold: true
                        color: Theme.accent
                    }
                }
            }

            // ==================== QUICK ACCESS GRID (Top Picks) ====================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                Text {
                    text: Strings.get("home_quick_picks") || "Acceso rápido"
                    font.family: Theme.fontFamily
                    font.pixelSize: 18
                    font.bold: true
                    color: Theme.textLight
                }

                GridLayout {
                    Layout.fillWidth: true
                    columns: root.width > 900 ? 3 : (root.width > 600 ? 2 : 1)
                    rowSpacing: 10
                    columnSpacing: 10

                    // Quick Pick 1: Tus Favoritos
                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 56
                        radius: Theme.radiusMd
                        color: favHover.containsMouse ?
                            Theme.withAlpha(Theme.bgSurface, 0.9) :
                            Theme.withAlpha(Theme.bgCard, 0.8)
                        border.color: Theme.borderDark
                        border.width: 1
                        clip: true

                        Behavior on color { ColorAnimation { duration: 140 } }

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 6
                            spacing: 12

                            Rectangle {
                                Layout.preferredWidth: 44
                                Layout.preferredHeight: 44
                                radius: 8
                                gradient: Gradient {
                                    GradientStop { position: 0.0; color: "#E0245E" }
                                    GradientStop { position: 1.0; color: "#8E05C2" }
                                }
                                AppIcon {
                                    anchors.centerIn: parent
                                    name: "heart"; size: 20
                                    iconColor: "#FFFFFF"
                                }
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text {
                                    text: Strings.get("nav_favorites") || "Tus Favoritos"
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 13
                                    font.bold: true
                                    color: Theme.textLight
                                    elide: Text.ElideRight
                                }
                                Text {
                                    text: Strings.get("home_synced_badge") || "Colección guardada"
                                    font.family: Theme.fontFamily
                                    font.pixelSize: 10
                                    color: Theme.textMuted
                                    elide: Text.ElideRight
                                }
                            }

                            Rectangle {
                                Layout.preferredWidth: 32
                                Layout.preferredHeight: 32
                                radius: 16
                                color: Theme.primary
                                opacity: favHover.containsMouse ? 1.0 : 0.0
                                Behavior on opacity { NumberAnimation { duration: 160 } }

                                AppIcon {
                                    anchors.centerIn: parent
                                    name: "play"; size: 14
                                    iconColor: Theme.bgDarkest
                                }
                            }
                        }

                        MouseArea {
                            id: favHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                var favs = [];
                                var all = root.libraryCtrl ? root.libraryCtrl.tracks : [];
                                for (var i = 0; i < all.length; ++i) {
                                    if (root.playlistCtrl && root.playlistCtrl.isLiked(all[i].id))
                                        favs.push(all[i]);
                                }
                                if (favs.length > 0 && root.queueCtrl)
                                    root.queueCtrl.playFromLibrary(favs, 0);
                            }
                        }
                    }

                    // Quick Pick 2..N: Top Synced Playlists
                    Repeater {
                        model: root.playlistCtrl ? root.playlistCtrl.effectivePlaylists.slice(0, 5) : []
                        delegate: Rectangle {
                            id: plCard
                            Layout.fillWidth: true
                            Layout.preferredHeight: 56
                            radius: Theme.radiusMd
                            color: plHover.containsMouse ?
                                Theme.withAlpha(Theme.bgSurface, 0.9) :
                                Theme.withAlpha(Theme.bgCard, 0.8)
                            border.color: Theme.borderDark
                            border.width: 1
                            clip: true

                            Behavior on color { ColorAnimation { duration: 140 } }

                            RowLayout {
                                anchors.fill: parent
                                anchors.margins: 6
                                spacing: 12

                                Rectangle {
                                    Layout.preferredWidth: 44
                                    Layout.preferredHeight: 44
                                    radius: 8
                                    color: Theme.bgDarkest
                                    border.color: Theme.borderDark
                                    border.width: 0.5
                                    AppIcon {
                                        anchors.centerIn: parent
                                        name: "list-music"; size: 20
                                        iconColor: Theme.primary
                                    }
                                }

                                ColumnLayout {
                                    Layout.fillWidth: true
                                    spacing: 2
                                    Text {
                                        text: modelData.nombre || "Playlist"
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 13
                                        font.bold: true
                                        color: Theme.textLight
                                        elide: Text.ElideRight
                                    }
                                    Text {
                                        text: (modelData.total_pistas || 0) + " pistas"
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 10
                                        color: Theme.textMuted
                                        elide: Text.ElideRight
                                    }
                                }

                                Rectangle {
                                    Layout.preferredWidth: 32
                                    Layout.preferredHeight: 32
                                    radius: 16
                                    color: Theme.primary
                                    opacity: plHover.containsMouse ? 1.0 : 0.0
                                    Behavior on opacity { NumberAnimation { duration: 160 } }

                                    AppIcon {
                                        anchors.centerIn: parent
                                        name: "play"; size: 14
                                        iconColor: Theme.bgDarkest
                                    }
                                }
                            }

                            MouseArea {
                                id: plHover
                                anchors.fill: parent
                                hoverEnabled: true
                                cursorShape: Qt.PointingHandCursor
                                onClicked: {
                                    var pistas = modelData.pistas || [];
                                    if (pistas.length > 0 && root.queueCtrl) {
                                        root.queueCtrl.playFromLibrary(pistas, 0);
                                    }
                                }
                            }
                        }
                    }
                }
            }

            // ==================== ÚLTIMOS MIXES (Your Top Mixes) ====================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: Strings.get("home_top_mixes") || "Últimos mixes"
                        font.family: Theme.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                        color: Theme.textLight
                    }
                    Item { Layout.fillWidth: true }
                }

                ListView {
                    id: mixList
                    Layout.fillWidth: true
                    Layout.preferredHeight: 205
                    orientation: ListView.Horizontal
                    spacing: 14
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds

                    // Generate distinct seed mixes from unique artists in the library
                    model: {
                        var list = [];
                        var seenArtists = {};
                        var tracks = root.libraryCtrl ? root.libraryCtrl.tracks : [];
                        for (var i = 0; i < tracks.length; ++i) {
                            var t = tracks[i];
                            var art = (t.artist || t.artista || "").trim();
                            if (art && !seenArtists[art]) {
                                seenArtists[art] = true;
                                list.push(t);
                                if (list.length >= 8) break;
                            }
                        }
                        return list;
                    }

                    delegate: Rectangle {
                        id: mixCard
                        width: 155
                        height: 200
                        radius: Theme.radiusLg
                        color: mixCardHover.containsMouse ? Theme.bgSurface : Theme.bgCard
                        border.color: Theme.borderDark
                        border.width: 1
                        clip: true

                        Behavior on color { ColorAnimation { duration: 150 } }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 8

                            // Mix artwork with gradient & music glyph
                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 125
                                radius: Theme.radiusMd
                                clip: true
                                gradient: Gradient {
                                    GradientStop {
                                        position: 0.0
                                        color: Qt.hsla((index * 0.17) % 1.0, 0.7, 0.28, 1.0)
                                    }
                                    GradientStop {
                                        position: 1.0
                                        color: Qt.hsla(((index * 0.17) + 0.1) % 1.0, 0.8, 0.12, 1.0)
                                    }
                                }

                                RoundedImage {
                                    anchors.fill: parent
                                    source: modelData.cover || modelData.coverUrl || ""
                                    radius: Theme.radiusMd
                                    visible: source.toString().length > 0
                                }

                                Rectangle {
                                    anchors.fill: parent
                                    radius: Theme.radiusMd
                                    color: Qt.rgba(0, 0, 0, 0.25)
                                    visible: (modelData.cover || "").length > 0
                                }

                                Column {
                                    anchors.left: parent.left; anchors.right: parent.right
                                    anchors.bottom: parent.bottom; anchors.margins: 8
                                    spacing: 2

                                    Rectangle {
                                        width: mixBadgeText.implicitWidth + 8
                                        height: 16; radius: 4
                                        color: Qt.rgba(0, 0, 0, 0.6)
                                        Text {
                                            id: mixBadgeText
                                            anchors.centerIn: parent
                                            text: "MIX"
                                            font.family: Theme.fontFamily
                                            font.pixelSize: 9
                                            font.bold: true
                                            color: Theme.primary
                                        }
                                    }

                                    Text {
                                        text: (modelData.artist || modelData.artista || "Dopamina") + " Mix"
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 12
                                        font.bold: true
                                        color: "#FFFFFF"
                                        elide: Text.ElideRight
                                        width: parent.width
                                    }
                                }

                                // Play button
                                Rectangle {
                                    anchors.right: parent.right; anchors.bottom: parent.bottom
                                    anchors.margins: 8
                                    width: 34; height: 34; radius: 17
                                    color: Theme.primary
                                    opacity: mixCardHover.containsMouse ? 1.0 : 0.0
                                    scale: mixCardHover.containsMouse ? 1.0 : 0.8
                                    Behavior on opacity { NumberAnimation { duration: 160 } }
                                    Behavior on scale { NumberAnimation { duration: 160 } }

                                    AppIcon {
                                        anchors.centerIn: parent
                                        name: "play"; size: 15
                                        iconColor: Theme.bgDarkest
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.title || modelData.titulo || "Pista"
                                font.family: Theme.fontFamily
                                font.pixelSize: 12
                                font.bold: true
                                color: Theme.textLight
                                elide: Text.ElideRight
                            }

                            Text {
                                Layout.fillWidth: true
                                text: "Mix por Dopamina"
                                font.family: Theme.fontFamily
                                font.pixelSize: 10
                                color: Theme.textMuted
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            id: mixCardHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.queueCtrl) {
                                    root.queueCtrl.startMixTrack(modelData);
                                }
                            }
                        }
                    }
                }
            }

            // ==================== NOVEDADES EN EL SERVIDOR (New Server Content) ====================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm
                visible: root.isOnline && root.libraryCtrl &&
                         root.libraryCtrl.recentTracks &&
                         root.libraryCtrl.recentTracks.length > 0

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: Strings.get("home_new_server_content") || "Novedades en el servidor"
                        font.family: Theme.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                        color: Theme.textLight
                    }
                    StatusPill {
                        text: "NUEVO"
                        pillColor: Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.2)
                        textColor: Theme.primary
                    }
                    Item { Layout.fillWidth: true }
                }

                ListView {
                    id: recentServerList
                    Layout.fillWidth: true
                    Layout.preferredHeight: 195
                    orientation: ListView.Horizontal
                    spacing: 14
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: root.libraryCtrl ? root.libraryCtrl.recentTracks : []

                    delegate: Rectangle {
                        id: recentCard
                        width: 145
                        height: 190
                        radius: Theme.radiusLg
                        color: recentHover.containsMouse ? Theme.bgSurface : Theme.bgCard
                        border.color: Theme.borderDark
                        border.width: 1
                        clip: true

                        Behavior on color { ColorAnimation { duration: 150 } }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 115
                                radius: Theme.radiusMd
                                color: Theme.bgDarkest
                                clip: true

                                RoundedImage {
                                    anchors.fill: parent
                                    source: modelData.cover || modelData.coverUrl || ""
                                    radius: Theme.radiusMd
                                }

                                AppIcon {
                                    anchors.centerIn: parent
                                    name: "music"; size: 28
                                    iconColor: Theme.primary
                                    visible: (modelData.cover || "").length === 0
                                }

                                // Audio Quality Pill
                                Rectangle {
                                    anchors.top: parent.top; anchors.left: parent.left
                                    anchors.margins: 6
                                    height: 16
                                    width: qualityLabel.implicitWidth + 8
                                    radius: 4
                                    color: Qt.rgba(0, 0, 0, 0.7)
                                    Text {
                                        id: qualityLabel
                                        anchors.centerIn: parent
                                        text: (modelData.formato || "FLAC").toUpperCase()
                                        font.family: Theme.fontFamily
                                        font.pixelSize: 8
                                        font.bold: true
                                        color: Theme.accent
                                    }
                                }

                                // Play button overlay
                                Rectangle {
                                    anchors.right: parent.right; anchors.bottom: parent.bottom
                                    anchors.margins: 6
                                    width: 32; height: 32; radius: 16
                                    color: Theme.primary
                                    opacity: recentHover.containsMouse ? 1.0 : 0.0
                                    scale: recentHover.containsMouse ? 1.0 : 0.8
                                    Behavior on opacity { NumberAnimation { duration: 160 } }
                                    Behavior on scale { NumberAnimation { duration: 160 } }

                                    AppIcon {
                                        anchors.centerIn: parent
                                        name: "play"; size: 14
                                        iconColor: Theme.bgDarkest
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.title || modelData.titulo || "Pista"
                                font.family: Theme.fontFamily
                                font.pixelSize: 12
                                font.bold: true
                                color: Theme.textLight
                                elide: Text.ElideRight
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.artist || modelData.artista || "Desconocido"
                                font.family: Theme.fontFamily
                                font.pixelSize: 10
                                color: Theme.textMuted
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            id: recentHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                if (root.queueCtrl) {
                                    root.queueCtrl.playNow(modelData);
                                }
                            }
                        }
                    }
                }
            }

            // ==================== PLAYLISTS SINCRONIZADAS ====================
            ColumnLayout {
                Layout.fillWidth: true
                spacing: Theme.spaceSm

                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: Strings.get("home_synced_playlists") || "Playlists sincronizadas"
                        font.family: Theme.fontFamily
                        font.pixelSize: 18
                        font.bold: true
                        color: Theme.textLight
                    }
                    Item { Layout.fillWidth: true }
                }

                ListView {
                    id: syncedPlaylistsList
                    Layout.fillWidth: true
                    Layout.preferredHeight: 180
                    orientation: ListView.Horizontal
                    spacing: 14
                    clip: true
                    boundsBehavior: Flickable.StopAtBounds
                    model: root.playlistCtrl ? root.playlistCtrl.effectivePlaylists : []

                    delegate: Rectangle {
                        id: plItemCard
                        width: 140
                        height: 175
                        radius: Theme.radiusLg
                        color: plItemHover.containsMouse ? Theme.bgSurface : Theme.bgCard
                        border.color: Theme.borderDark
                        border.width: 1
                        clip: true

                        Behavior on color { ColorAnimation { duration: 150 } }

                        ColumnLayout {
                            anchors.fill: parent
                            anchors.margins: 10
                            spacing: 6

                            Rectangle {
                                Layout.fillWidth: true
                                Layout.preferredHeight: 110
                                radius: Theme.radiusMd
                                color: Theme.bgDarkest
                                clip: true

                                AppIcon {
                                    anchors.centerIn: parent
                                    name: "list-music"; size: 32
                                    iconColor: Theme.primary
                                }

                                // Play button overlay
                                Rectangle {
                                    anchors.right: parent.right; anchors.bottom: parent.bottom
                                    anchors.margins: 6
                                    width: 32; height: 32; radius: 16
                                    color: Theme.primary
                                    opacity: plItemHover.containsMouse ? 1.0 : 0.0
                                    scale: plItemHover.containsMouse ? 1.0 : 0.8
                                    Behavior on opacity { NumberAnimation { duration: 160 } }
                                    Behavior on scale { NumberAnimation { duration: 160 } }

                                    AppIcon {
                                        anchors.centerIn: parent
                                        name: "play"; size: 14
                                        iconColor: Theme.bgDarkest
                                    }
                                }
                            }

                            Text {
                                Layout.fillWidth: true
                                text: modelData.nombre || "Playlist"
                                font.family: Theme.fontFamily
                                font.pixelSize: 12
                                font.bold: true
                                color: Theme.textLight
                                elide: Text.ElideRight
                            }

                            Text {
                                Layout.fillWidth: true
                                text: (modelData.total_pistas || 0) + " canciones"
                                font.family: Theme.fontFamily
                                font.pixelSize: 10
                                color: Theme.textMuted
                                elide: Text.ElideRight
                            }
                        }

                        MouseArea {
                            id: plItemHover
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                var pistas = modelData.pistas || [];
                                if (pistas.length > 0 && root.queueCtrl)
                                    root.queueCtrl.playFromLibrary(pistas, 0);
                            }
                        }
                    }
                }
            }
        }
    }
}

