/**
 * Location: src/features/library/ui/TrackContextMenu.qml
 * Function: Rich track context menu triggered by right-click or 3-dots button,
 * displaying track cover header and all playback, queue, playlist, navigation,
 * Hi-Fi telemetry, and download actions.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"

Popup {
    id: root
    parent: Overlay.overlay
    width: 252
    padding: 8
    modal: false
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var track: null
    property bool isLiked: false

    readonly property int trackId: root.track ? (root.track.id || 0) : 0
    readonly property string trackTitle: {
        if (!root.track) return "";
        return root.track.titulo || root.track.title || root.track.name || "";
    }
    readonly property string trackArtist: {
        if (!root.track) return "";
        return root.track.artista || root.track.artist || "";
    }
    readonly property string trackAlbum: {
        if (!root.track) return "";
        return root.track.album || "Sencillo";
    }
    readonly property string coverUrl: {
        if (!root.track) return "";
        var c = root.track.coverUrl || root.track.cover_url ||
                root.track.cover || "";
        return (c.indexOf("http") === 0 || c.indexOf("file://") === 0) ? c : "";
    }

    function openAtItem(sourceItem, localX, localY) {
        if (!Overlay.overlay || !sourceItem) {
            root.open();
            return;
        }
        var pt = sourceItem.mapToItem(Overlay.overlay, localX, localY);
        var ox = pt.x;
        var oy = pt.y;
        var maxW = Overlay.overlay.width;
        var maxH = Overlay.overlay.height;
        var menuH = 442;
        if (ox + root.width > maxW - 10) ox = Math.max(10, maxW - root.width - 10);
        if (oy + menuH > maxH - 10) oy = Math.max(10, maxH - menuH - 10);
        root.x = Math.round(ox);
        root.y = Math.round(oy);
        root.open();
    }

    background: Rectangle {
        radius: 12
        color: "#16161D"
        border.color: "#2B2B38"
        border.width: 1
    }

    component MenuDivider: Rectangle {
        Layout.fillWidth: true
        Layout.topMargin: 4
        Layout.bottomMargin: 4
        height: 1
        color: "#262632"
    }

    component MenuRow: Rectangle {
        id: rowRoot
        property string iconType: ""
        property string label: ""
        signal triggered()

        Layout.fillWidth: true
        Layout.preferredHeight: rowText.lineCount > 1 ? 42 : 32
        radius: 7
        color: rowHover.containsMouse ? "#252532" : "transparent"

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 10
            anchors.rightMargin: 10
            spacing: 11

            Canvas {
                id: iconCanvas
                Layout.preferredWidth: 16
                Layout.preferredHeight: 16
                onPaint: {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.strokeStyle = rowHover.containsMouse ? "#FFFFFF" : "#A6A6B6";
                    ctx.fillStyle = ctx.strokeStyle;
                    ctx.lineWidth = 1.75;
                    ctx.lineCap = "round";
                    ctx.lineJoin = "round";
                    var t = rowRoot.iconType;
                    if (t === "mix") {
                        ctx.beginPath();
                        ctx.moveTo(8, 2); ctx.lineTo(8, 14);
                        ctx.moveTo(11.5, 4); ctx.lineTo(11.5, 12);
                        ctx.moveTo(4.5, 5.5); ctx.lineTo(4.5, 10.5);
                        ctx.moveTo(14.5, 6); ctx.lineTo(14.5, 10);
                        ctx.moveTo(1.5, 7); ctx.lineTo(1.5, 9);
                        ctx.stroke();
                    } else if (t === "radio") {
                        ctx.beginPath();
                        ctx.arc(8, 8, 1.5, 0, Math.PI * 2);
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.arc(8, 8, 4.2, -0.75, 0.75);
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.arc(8, 8, 4.2, Math.PI - 0.75, Math.PI + 0.75);
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.arc(8, 8, 6.8, -0.75, 0.75);
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.arc(8, 8, 6.8, Math.PI - 0.75, Math.PI + 0.75);
                        ctx.stroke();
                    } else if (t === "play-next") {
                        ctx.beginPath();
                        ctx.moveTo(3, 2.5); ctx.lineTo(11.5, 8);
                        ctx.lineTo(3, 13.5); ctx.closePath();
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.moveTo(13.2, 3); ctx.lineTo(13.2, 13);
                        ctx.stroke();
                    } else if (t === "queue") {
                        ctx.beginPath();
                        ctx.moveTo(4.5, 4); ctx.lineTo(13.5, 4);
                        ctx.moveTo(4.5, 8); ctx.lineTo(13.5, 8);
                        ctx.moveTo(4.5, 12); ctx.lineTo(13.5, 12);
                        ctx.stroke();
                    } else if (t === "save") {
                        ctx.beginPath();
                        ctx.moveTo(3, 2.5); ctx.lineTo(10.5, 2.5);
                        ctx.lineTo(13.5, 5.5); ctx.lineTo(13.5, 13.5);
                        ctx.lineTo(3, 13.5); ctx.closePath();
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.moveTo(5.5, 13.5); ctx.lineTo(5.5, 9);
                        ctx.lineTo(11, 9); ctx.lineTo(11, 13.5);
                        ctx.stroke();
                    } else if (t === "heart") {
                        ctx.beginPath();
                        ctx.moveTo(8, 13.5);
                        ctx.bezierCurveTo(2, 9.5, 2, 4, 5.2, 3.2);
                        ctx.bezierCurveTo(6.8, 2.8, 8, 4.5, 8, 4.5);
                        ctx.bezierCurveTo(8, 4.5, 9.2, 2.8, 10.8, 3.2);
                        ctx.bezierCurveTo(14, 4, 14, 9.5, 8, 13.5);
                        ctx.stroke();
                    } else if (t === "user") {
                        ctx.beginPath();
                        ctx.arc(8, 5, 2.7, 0, Math.PI * 2);
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.arc(8, 14.5, 5.2, Math.PI * 1.12, Math.PI * 1.88);
                        ctx.stroke();
                    } else if (t === "disc") {
                        ctx.beginPath();
                        ctx.arc(8, 8, 6.2, 0, Math.PI * 2);
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.arc(8, 8, 1.8, 0, Math.PI * 2);
                        ctx.stroke();
                    } else if (t === "info") {
                        ctx.beginPath();
                        ctx.arc(8, 8, 6.2, 0, Math.PI * 2);
                        ctx.stroke();
                        ctx.beginPath();
                        ctx.moveTo(8, 7.5); ctx.lineTo(8, 11.2);
                        ctx.moveTo(8, 5.2); ctx.lineTo(8.05, 5.2);
                        ctx.stroke();
                    } else if (t === "download") {
                        ctx.beginPath();
                        ctx.moveTo(8, 2.5); ctx.lineTo(8, 10.2);
                        ctx.moveTo(5, 7.5); ctx.lineTo(8, 10.5); ctx.lineTo(11, 7.5);
                        ctx.moveTo(2.5, 11); ctx.lineTo(2.5, 13.5);
                        ctx.lineTo(13.5, 13.5); ctx.lineTo(13.5, 11);
                        ctx.stroke();
                    }
                }
                Connections {
                    target: rowHover
                    function onContainsMouseChanged() { iconCanvas.requestPaint(); }
                }
            }

            Text {
                id: rowText
                text: rowRoot.label
                font.family: Theme.fontFamily
                font.pixelSize: 12
                font.bold: true
                color: rowHover.containsMouse ? "#FFFFFF" : "#EAEAF2"
                wrapMode: Text.WordWrap
                Layout.fillWidth: true
            }
        }

        MouseArea {
            id: rowHover
            anchors.fill: parent
            hoverEnabled: true
            cursorShape: Qt.PointingHandCursor
            onClicked: {
                root.close();
                rowRoot.triggered();
            }
        }
    }

    contentItem: ColumnLayout {
        spacing: 2

        RowLayout {
            Layout.fillWidth: true
            Layout.leftMargin: 6
            Layout.rightMargin: 6
            Layout.topMargin: 4
            Layout.bottomMargin: 4
            spacing: 10

            Rectangle {
                Layout.preferredWidth: 38
                Layout.preferredHeight: 38
                radius: 6
                color: "#1E1E28"
                border.color: "#2E2E3E"
                border.width: 1

                AppIcon {
                    anchors.centerIn: parent
                    name: "disc"; size: 16
                    iconColor: Theme.textDim
                }
                RoundedImage {
                    anchors.fill: parent
                    radius: 6
                    source: root.coverUrl
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 1

                Text {
                    text: root.trackTitle
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.bold: true
                    color: "#FFFFFF"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    text: root.trackArtist + " \u2022 " + root.trackAlbum
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    color: "#9696A8"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }
        }

        MenuDivider {}

        MenuRow {
            iconType: "mix"
            label: Strings.get("ctx_play_mix") || "Oír mix de la canción"
            onTriggered: {
                if (typeof queueController !== "undefined" && queueController &&
                    root.track) {
                    queueController.startMixTrack(root.track);
                }
            }
        }

        MenuRow {
            iconType: "radio"
            label: Strings.get("ctx_go_radio") || "Ir a radio de la canción"
            onTriggered: {
                if (typeof queueController !== "undefined" && queueController &&
                    root.track) {
                    queueController.startRadioTrack(root.track);
                }
            }
        }

        MenuDivider {}

        MenuRow {
            iconType: "play-next"
            label: Strings.get("ctx_play_next") || "Reproducir a continuación"
            onTriggered: {
                if (typeof queueController !== "undefined" && queueController &&
                    root.track) {
                    queueController.playNext(root.track);
                }
            }
        }

        MenuRow {
            iconType: "queue"
            label: Strings.get("ctx_add_queue") || "Agregar a la fila de reproducción"
            onTriggered: {
                if (typeof queueController !== "undefined" && queueController &&
                    root.track) {
                    queueController.appendToQueue(root.track);
                }
            }
        }

        MenuRow {
            iconType: "save"
            label: Strings.get("ctx_add_playlist") || "Agregar a playlist..."
            onTriggered: addToPlaylistModal.openForTrack(root.track)
        }

        MenuRow {
            iconType: "heart"
            label: root.isLiked ?
                (Strings.get("ctx_remove_liked") || "Quitar de Tus me gusta") :
                (Strings.get("ctx_save_liked") || "Guardar en Tus me gusta")
            onTriggered: {
                if (typeof playlistController !== "undefined" &&
                    playlistController && root.trackId !== 0) {
                    playlistController.toggleLike(root.trackId);
                }
            }
        }

        MenuDivider {}

        MenuRow {
            iconType: "user"
            label: Strings.get("ctx_go_artist") || "Ir al artista"
            onTriggered: {
                if (typeof libraryController !== "undefined" &&
                    libraryController && root.trackArtist.length > 0) {
                    libraryController.search(root.trackArtist);
                }
            }
        }

        MenuRow {
            iconType: "disc"
            label: Strings.get("ctx_go_album") || "Ir al álbum"
            onTriggered: {
                if (typeof libraryController !== "undefined" &&
                    libraryController && root.trackAlbum.length > 0) {
                    libraryController.search(root.trackAlbum);
                }
            }
        }

        MenuDivider {}

        MenuRow {
            iconType: "info"
            label: Strings.get("ctx_hifi_details") || "Detalles técnicos y acústicos Hi-Fi"
            onTriggered: technicalDialog.openForTrack(root.track)
        }

        MenuRow {
            iconType: "download"
            label: Strings.get("ctx_download_audio") || "Descargar archivo de audio"
            onTriggered: {
                if (typeof libraryController !== "undefined" &&
                    libraryController && root.track) {
                    libraryController.downloadTrack(
                        root.trackId, root.trackTitle,
                        root.track.format || root.track.formato || "flac",
                        root.track.path || "");
                }
            }
        }
    }

    TrackTechnicalDialog { id: technicalDialog }
    AddToPlaylistDialog { id: addToPlaylistModal }
}
