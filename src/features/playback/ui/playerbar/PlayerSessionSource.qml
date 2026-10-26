/**
 * Location: src/features/playback/ui/playerbar/PlayerSessionSource.qml
 * Function: Compact badge identifying the client whose playback queue is being
 * mirrored on desktop, plus whether that remote client is still playing.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "../../../../shared/ui"

RowLayout {
    id: root
    spacing: Theme.spaceXs
    visible: sessionPlaybackController.hasSession

    AppIcon {
        name: "wifi"
        size: 10
        iconColor: Theme.accent
        Layout.alignment: Qt.AlignVCenter
    }

    Text {
        Layout.fillWidth: true
        text: {
            var source = sessionPlaybackController.sourceKind === "mobile" ?
                Strings.get("session_source_mobile") :
                Strings.get("session_source_web")
            if (sessionPlaybackController.remotePlaying)
                source += " · " + Strings.get("session_source_playing")
            return source
        }
        font.family: Theme.fontFamily
        font.pixelSize: Theme.fontSm
        font.letterSpacing: 0.2
        color: Theme.accent
        elide: Text.ElideRight
    }
}
