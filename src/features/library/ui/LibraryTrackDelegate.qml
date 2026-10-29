/**
 * Location: src/features/library/ui/LibraryTrackDelegate.qml
 * Function: Alphabetical Hi-Fi track card delegate rendering section headers,
 * rounded album artwork, title + "Artist • Album", format badge pill
 * (e.g. FLAC 24b/96.0k), duration, favorite heart toggle, and options menu.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"

Item {
    id: root
    width: parent ? parent.width : 600
    height: (showHeader ? 24 : 0) + 64

    property var track: modelData
    property bool isTracksSection: true
    property int currentPlayingId: 0
    property string currentPlayingTitle: ""

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
        return root.track.album || "";
    }
    readonly property string subtitleText: {
        if (root.trackArtist.length > 0 && root.trackAlbum.length > 0)
            return root.trackArtist + " \u2022 " + root.trackAlbum;
        return root.trackArtist.length > 0 ? root.trackArtist : root.trackAlbum;
    }
    readonly property double trackDuration: {
        if (!root.track) return 0;
        return root.track.duracion || root.track.duration || 0;
    }
    readonly property string formatBadge: {
        if (!root.track) return "FLAC 24b/96.0k";
        return root.track.formatBadge || "FLAC 24b/96.0k";
    }
    readonly property string sectionLetter: {
        if (!root.track) return "";
        return root.track.section || "";
    }
    readonly property bool showHeader:
        root.isTracksSection && root.track &&
        Boolean(root.track.showSectionHeader) &&
        root.sectionLetter.length > 0

    readonly property bool isCurrentTrack: {
        if (root.trackId !== 0 && root.currentPlayingId !== 0) {
            return root.currentPlayingId === root.trackId;
        }
        return root.currentPlayingTitle.length > 0 &&
               root.currentPlayingTitle === root.trackTitle;
    }

    property int likeRev: 0
    readonly property bool isLiked: {
        void(root.likeRev);
        return typeof playlistController !== "undefined" &&
               playlistController !== null &&
               root.trackId !== 0 &&
               playlistController.isLiked(root.trackId);
    }

    Connections {
        target: typeof playlistController !== "undefined" ?
            playlistController : null
        function onLikesChanged() { root.likeRev += 1; }
    }

    readonly property string coverUrl: {
        if (!root.track) return "";
        var c = root.track.coverUrl || root.track.cover_url ||
                root.track.cover || "";
        return (c.indexOf("http") === 0 || c.indexOf("file://") === 0) ? c : "";
    }

    signal playRequested(var t)
    signal mixRequested(var t)

    Text {
        id: sectionHeader
        visible: root.showHeader
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.leftMargin: 10
        height: root.showHeader ? 24 : 0
        verticalAlignment: Text.AlignVCenter
        text: root.sectionLetter
        font.family: Theme.fontFamily
        font.pixelSize: 13
        font.bold: true
        color: Theme.accent
    }

    Rectangle {
        id: card
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: 64
        radius: 12
        color: root.isCurrentTrack ?
            Qt.rgba(Theme.primary.r, Theme.primary.g, Theme.primary.b, 0.16) :
            (cardHover.containsMouse ? "#181822" : "#111116")
        border.width: root.isCurrentTrack ? 1.2 : 1
        border.color: root.isCurrentTrack ?
            Theme.primary :
            (cardHover.containsMouse ? "#323244" : "#22222C")

        Behavior on color { ColorAnimation { duration: 160 } }
        Behavior on border.color { ColorAnimation { duration: 160 } }

        MouseArea {
            id: cardHover
            anchors.fill: parent
            hoverEnabled: true
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            cursorShape: Qt.PointingHandCursor
            onClicked: mouse => {
                if (mouse.button === Qt.RightButton) {
                    trackMenu.openAtItem(card, mouse.x, mouse.y);
                } else {
                    root.playRequested(root.track);
                }
            }
            onDoubleClicked: mouse => {
                if (mouse.button === Qt.LeftButton) {
                    root.playRequested(root.track);
                }
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 12
            anchors.rightMargin: 14
            spacing: 14

            Rectangle {
                Layout.preferredWidth: 44
                Layout.preferredHeight: 44
                radius: 8
                color: "#1B1B24"
                border.color: "#2A2A36"
                border.width: 1

                AppIcon {
                    anchors.centerIn: parent
                    name: "disc"; size: 20
                    iconColor: Theme.textDim
                }
                RoundedImage {
                    anchors.fill: parent
                    radius: 8
                    source: root.coverUrl
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: 2

                Text {
                    text: root.trackTitle
                    font.family: Theme.fontFamily
                    font.pixelSize: 14
                    font.bold: true
                    color: root.isCurrentTrack ? Theme.primary : "#F5F5FA"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }

                Text {
                    visible: root.subtitleText.length > 0
                    text: root.subtitleText
                    font.family: Theme.fontFamily
                    font.pixelSize: 12
                    color: "#9494A6"
                    elide: Text.ElideRight
                    Layout.fillWidth: true
                }
            }

            AudioQualityBadge {
                visible: root.isTracksSection
                track: root.track
            }

            Text {
                visible: root.isTracksSection
                text: Theme.formatDuration(root.trackDuration)
                font.family: Theme.fontFamily
                font.pixelSize: 13
                font.bold: true
                color: "#E8E8F0"
                Layout.preferredWidth: 42
                horizontalAlignment: Text.AlignRight
            }

            Item {
                visible: root.isTracksSection
                Layout.preferredWidth: 28
                Layout.preferredHeight: 28

                AppIcon {
                    anchors.centerIn: parent
                    name: "heart"
                    size: 16
                    iconColor: root.isLiked ? Theme.primary :
                        (heartArea.containsMouse ? "#FFFFFF" : "#A0A0B0")
                }

                MouseArea {
                    id: heartArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        if (typeof playlistController !== "undefined" &&
                            playlistController && root.trackId !== 0) {
                            playlistController.toggleLike(root.trackId);
                        }
                    }
                }
            }

            Item {
                id: menuBtn
                visible: root.isTracksSection
                Layout.preferredWidth: 24
                Layout.preferredHeight: 28

                Column {
                    anchors.centerIn: parent
                    spacing: 3
                    Repeater {
                        model: 3
                        Rectangle {
                            width: 3.5; height: 3.5; radius: 1.75
                            color: moreArea.containsMouse ? "#FFFFFF" : "#A0A0B0"
                        }
                    }
                }

                MouseArea {
                    id: moreArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: trackMenu.openAtItem(
                        menuBtn, -220, menuBtn.height + 4)
                }

                TrackContextMenu {
                    id: trackMenu
                    track: root.track
                    isLiked: root.isLiked
                }
            }
        }
    }
}
