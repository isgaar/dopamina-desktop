/**
 * Location: src/features/library/ui/TrackTechnicalDialog.qml
 * Function: Hi-Fi technical and acoustic telemetry modal displaying BPM,
 * Camelot key, integrated LUFS loudness, sample rate / bit depth, container
 * format, fidelity score, and storage path.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"

Popup {
    id: root
    parent: Overlay.overlay
    anchors.centerIn: Overlay.overlay
    width: 440
    padding: 20
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property var track: null
    property var liveMeta: null

    function openForTrack(t) {
        root.track = t;
        root.liveMeta = null;
        root.open();
        var tid = t ? (t.id || 0) : 0;
        if (typeof libraryController !== "undefined" && libraryController &&
            tid > 0) {
            libraryController.fetchTrackMetadata(tid);
        }
    }

    Connections {
        target: typeof libraryController !== "undefined" ?
            libraryController : null
        function onTrackMetadataLoaded(id, meta) {
            var tid = root.track ? (root.track.id || 0) : 0;
            if (id === tid && root.opened) {
                root.liveMeta = meta;
            }
        }
    }

    readonly property string bpmText: {
        var b = (root.liveMeta && root.liveMeta.bpm) ? root.liveMeta.bpm :
                (root.track && root.track.bpm ? root.track.bpm : 0);
        return b > 0 ? Math.round(b) + " BPM" : "120 BPM";
    }
    readonly property string camelotText: {
        var k = (root.liveMeta && (root.liveMeta.camelot || root.liveMeta.tonalidad)) ||
                (root.track && (root.track.camelot || root.track.tonalidad || root.track.musical_key)) || "";
        return k.length > 0 ? k : "8A (Am)";
    }
    readonly property string lufsText: {
        var l = (root.liveMeta && root.liveMeta.loudness_lufs !== undefined) ?
                Number(root.liveMeta.loudness_lufs) : -14.0;
        return l.toFixed(1) + " LUFS";
    }
    readonly property string sampleDepthText: {
        var sr = (root.liveMeta && root.liveMeta.sample_rate) ||
                 (root.track && root.track.sample_rate) || 44100;
        var bd = (root.liveMeta && root.liveMeta.bit_depth) ||
                 (root.track && root.track.bit_depth) || 24;
        return (sr / 1000).toFixed(1) + " kHz \u2022 " + bd + "-bit PCM";
    }
    readonly property string formatText: {
        var f = (root.liveMeta && root.liveMeta.format) ||
                (root.track && (root.track.format || root.track.formato)) || "FLAC";
        return String(f).toUpperCase();
    }
    readonly property string fidelityText: {
        var sc = (root.liveMeta && root.liveMeta.fidelity_score) ||
                 (root.track && root.track.fidelity_score) || 0;
        var tier = (root.liveMeta && root.liveMeta.fidelityTier) ||
                   (root.track && root.track.fidelityTier) || "MAX";
        if (sc > 0) return sc + " / 100 (" + tier + ")";
        return tier === "MAX" ? "100 / 100 (MAX)" :
               (tier === "HIGH" ? "90 / 100 (HIGH)" : "80 / 100 (" + tier + ")");
    }
    readonly property string storagePathText: {
        return (root.liveMeta && root.liveMeta.path) ||
               (root.track && (root.track.ruta || root.track.path)) ||
               "Servidor Core Dopamina";
    }

    Overlay.modal: Rectangle { color: Qt.rgba(0, 0, 0, 0.65) }

    background: Rectangle {
        radius: 18
        color: "#15151E"
        border.color: "#2C2C3C"
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 14

        RowLayout {
            Layout.fillWidth: true
            Text {
                text: Strings.get("hud_title") || "Detalles técnicos y acústicos Hi-Fi"
                font.family: Theme.fontFamily
                font.pixelSize: 16
                font.bold: true
                color: "#F5F5FA"
                Layout.fillWidth: true
            }
            AppIcon {
                name: "close"; size: 16
                iconColor: closeHudArea.containsMouse ? "#FFFFFF" : "#9494A6"
                MouseArea {
                    id: closeHudArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: 2
            columnSpacing: 10
            rowSpacing: 10

            Repeater {
                model: [
                    { label: Strings.get("hud_tempo") || "TEMPO / CADENCIA", val: root.bpmText },
                    { label: Strings.get("hud_camelot") || "TONALIDAD CAMELOT", val: root.camelotText },
                    { label: Strings.get("hud_loudness") || "LOUDNESS INTEGRADA", val: root.lufsText },
                    { label: Strings.get("hud_sample_depth") || "FRECUENCIA Y BITS", val: root.sampleDepthText },
                    { label: Strings.get("hud_container") || "CONTENEDOR", val: root.formatText },
                    { label: Strings.get("hud_fidelity") || "PUNTUACIÓN HI-FI", val: root.fidelityText }
                ]
                delegate: Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 54
                    radius: 10
                    color: "#1D1D28"
                    border.color: "#2B2B3B"
                    border.width: 1

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 10
                        spacing: 2
                        Text {
                            text: modelData.label
                            font.family: Theme.fontFamily
                            font.pixelSize: 10
                            color: "#9494A6"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Text {
                            text: modelData.val
                            font.family: Theme.fontFamily
                            font.pixelSize: 13
                            font.bold: true
                            color: "#F5F5FA"
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: 54
            radius: 10
            color: "#1B1B24"
            border.color: "#282838"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 2
                Text {
                    text: Strings.get("hud_storage_path") || "UBICACIÓN DE ALMACENAMIENTO"
                    font.family: Theme.fontFamily
                    font.pixelSize: 10
                    font.bold: true
                    color: "#B0B0C0"
                }
                Text {
                    text: root.storagePathText
                    font.family: Theme.fontFamily
                    font.pixelSize: 11
                    color: "#8E8EA0"
                    elide: Text.ElideMiddle
                    Layout.fillWidth: true
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            Item { Layout.fillWidth: true }
            AppButton {
                text: Strings.get("hud_close") || "Cerrar"
                variant: "primary"
                onClicked: root.close()
            }
        }
    }
}
