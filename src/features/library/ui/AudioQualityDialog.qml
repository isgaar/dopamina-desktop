/**
 * Location: src/features/library/ui/AudioQualityDialog.qml
 * Function: Modal dialog displaying track audio fidelity tier (MAX, HIGH, OK,
 * LOW), format/bit-depth/sample-rate summary, and acoustic metadata matching
 * Android's NowPlayingAudioBadge dialog.
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
    width: 390
    padding: 20
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside

    property int trackId: 0
    property string tier: "MAX"
    property string qualityDetails: "FLAC 24-bit / 44 kHz"
    property string extraMeta: ""

    function openForTrack(t) {
        if (!t) return;
        root.trackId = t.id || 0;
        root.tier = t.fidelityTier || t.formatBadge || "MAX";
        root.qualityDetails = t.qualityDetails || "FLAC 24-bit / 44 kHz";
        root.extraMeta = t.extraMeta || "";
        root.open();
        if (typeof libraryController !== "undefined" && libraryController &&
            root.trackId > 0) {
            libraryController.fetchTrackMetadata(root.trackId);
        }
    }

    Connections {
        target: typeof libraryController !== "undefined" ?
            libraryController : null
        function onTrackMetadataLoaded(id, meta) {
            if (id !== root.trackId || !root.opened) return;
            if (meta.fidelityTier) root.tier = meta.fidelityTier;
            if (meta.qualityDetails) root.qualityDetails = meta.qualityDetails;
            if (meta.extraMeta && meta.extraMeta.length > 0)
                root.extraMeta = meta.extraMeta;
        }
    }

    readonly property string tierDescription: {
        var d = "";
        if (root.tier === "MAX") d = Strings.get("audio_quality_max_desc");
        else if (root.tier === "HIGH") d = Strings.get("audio_quality_high_desc");
        else if (root.tier === "LOW") d = Strings.get("audio_quality_low_desc");
        else d = Strings.get("audio_quality_ok_desc");
        if (d && d.length > 0) return d;
        if (root.tier === "MAX") return "Máxima fidelidad: Hi-Res Lossless real (24-bit / >48 kHz con armónicos ultrasónicos verificados)";
        if (root.tier === "HIGH") return "Alta fidelidad: Calidad CD sin pérdidas (16-bit / 44.1-48 kHz con espectro audible completo)";
        if (root.tier === "LOW") return "Baja fidelidad: Transcodificación con corte abrupto en 16 kHz o compresión a baja tasa de bits";
        return "Fidelidad aceptable: Audio comprimido a alta tasa de bits (~320 kbps o corte espectral superior)";
    }

    Overlay.modal: Rectangle {
        color: Qt.rgba(0, 0, 0, 0.62)
    }

    background: Rectangle {
        radius: 20
        color: "#16161F"
        border.color: "#2C2C3C"
        border.width: 1
    }

    contentItem: ColumnLayout {
        spacing: 12

        Text {
            Layout.fillWidth: true
            text: root.tier + " - " + root.qualityDetails
            font.family: Theme.fontFamily
            font.pixelSize: 16
            font.bold: true
            color: "#F5F5FA"
            wrapMode: Text.WordWrap
        }

        Text {
            Layout.fillWidth: true
            text: root.tierDescription
            font.family: Theme.fontFamily
            font.pixelSize: 13
            color: "#B0B0C0"
            wrapMode: Text.WordWrap
            lineHeight: 1.25
        }

        Text {
            visible: root.extraMeta.length > 0
            Layout.fillWidth: true
            text: root.extraMeta
            font.family: Theme.fontFamily
            font.pixelSize: 12
            font.bold: true
            color: Theme.accent
            wrapMode: Text.WordWrap
        }

        RowLayout {
            Layout.fillWidth: true
            Layout.topMargin: 4
            Item { Layout.fillWidth: true }

            Rectangle {
                height: 32
                width: dismissLbl.implicitWidth + 24
                radius: 8
                color: dismissArea.containsMouse ?
                    Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.16) :
                    "transparent"

                Text {
                    id: dismissLbl
                    anchors.centerIn: parent
                    text: Strings.get("dismiss") || "Aceptar"
                    font.family: Theme.fontFamily
                    font.pixelSize: 13
                    font.bold: true
                    color: Theme.primary
                }

                MouseArea {
                    id: dismissArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: root.close()
                }
            }
        }
    }
}
