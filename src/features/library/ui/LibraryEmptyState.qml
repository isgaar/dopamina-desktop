/**
 * Location: src/features/library/ui/LibraryEmptyState.qml
 * Function: Left-aligned library empty state matching the Android reference,
 * with an informational glyph, headline, and hint text.
 * All Rights Reserved isgaar
 */
import QtQuick
import "../../../shared/ui"

Item {
    id: root
    property string section: "tracks"

    readonly property bool isTracks: root.section === "tracks"
    readonly property string headline: isTracks ?
        Strings.get("tracks_empty_title") :
        Strings.get("empty_section_title")
    readonly property string hint: isTracks ?
        Strings.get("tracks_empty_desc") :
        Strings.get("empty_section_desc")

    Column {
        x: Theme.spaceMd; y: Theme.spaceMd
        width: Math.min(parent.width - Theme.spaceXl, 560)
        spacing: Theme.spaceSm

        AppIcon {
            name: "info"
            size: 28
            iconColor: Theme.textLight
        }

        Item { width: 1; height: 1 }

        Text {
            width: parent.width
            text: root.headline
            font.family: Theme.fontFamily
            font.pixelSize: 15
            font.bold: true
            color: Theme.textLight
            wrapMode: Text.WordWrap
        }

        Text {
            width: parent.width
            text: root.hint
            font.family: Theme.fontFamily
            font.pixelSize: 13
            color: Theme.textMuted
            wrapMode: Text.WordWrap
            lineHeight: 1.25
        }
    }
}
