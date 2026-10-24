/**
 * Location: src/shared/ui/AppIcon.qml
 * Function: Reusable vector icon component backed by the bundled Lucide icon
 * font for consistent interface iconography across the desktop client.
 * All Rights Reserved isgaar
 */
import QtQuick

Text {
    id: root
    property string name: ""
    property int size: 14
    property color iconColor: Theme.textLight

    font.family: "lucide"
    font.pixelSize: size
    color: iconColor
    horizontalAlignment: Text.AlignHCenter; verticalAlignment: Text.AlignVCenter

    text: {
        switch (name) {
        case "music": return "\ue126";
        case "music-2": return "\ue34e";
        case "disc": return "\ue0b3";
        case "disc-album": return "\ue561";
        case "list": return "\ue10c";
        case "list-music": return "\ue2e0";
        case "play": return "\ue140";
        case "circle-play": return "\ue085";
        case "pause": return "\ue132";
        case "stop":
        case "square": return "\ue16b";
        case "next":
        case "skip-forward": return "\ue164";
        case "mix":
        case "shuffle": return "\ue162";
        case "refresh":
        case "refresh-cw": return "\ue149";
        case "search": return "\ue155";
        case "filter": return "\ue0d5";
        case "check": return "\ue070";
        case "database": return "\ue0b1";
        case "sliders":
        case "sliders-horizontal": return "\ue29a";
        case "folder": return "\ue0dc";
        case "clock":
        case "clock-3": return "\ue250";
        case "moon": return "\ue122";
        case "image": return "\ue0f9";
        case "chevron-down": return "\ue071";
        case "right":
        case "chevron-right": return "\ue073";
        case "left":
        case "chevron-left": return "\ue072";
        case "info": return "\ue0fe";
        case "power": return "\ue144";
        case "star": return "\ue17a";
        case "server": return "\ue157";
        case "lock": return "\ue10f";
        case "bookmark": return "\ue064";
        case "rotate-ccw": return "\ue14c";
        case "settings": return "\ue158";
        case "users": return "\ue1a4";
        case "tag": return "\ue183";
        case "volume": return "\ue1a9";
        case "volume-2": return "\ue1ab";
        case "contrast": return "\ue0a1";
        case "plug": return "\ue383";
        case "eye": return "\ue0be";
        case "close":
        case "x": return "\ue1b2";
        case "palette": return "\ue1dd";
        case "globe": return "\ue0eb";
        case "circle-check": return "\ue226";
        case "circle-alert": return "\ue07b";
        case "triangle-alert": return "\ue193";
        default: return "\ue126";
        }
    }
}
