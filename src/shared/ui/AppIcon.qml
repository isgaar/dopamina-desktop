/**
 * Location: src/shared/ui/AppIcon.qml
 * Function: Reusable vector icon component backed by the Lucide icon library
 * font ensuring consistent cross-platform symbology for radio controls.
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
    horizontalAlignment: Text.AlignHCenter
    verticalAlignment: Text.AlignVCenter

    text: {
        switch (name) {
        case "music": return "\ue261";
        case "music-2": return "\ue162";
        case "radio": return "\ue146";
        case "radio-tower": return "\ue409";
        case "disc": return "\ue39b";
        case "disc-album": return "\ue39c";
        case "list-music": return "\ue145";
        case "play": return "\ue140";
        case "circle-play": return "\ue085";
        case "pause": return "\ue132";
        case "stop":
        case "square": return "\ue16b";
        case "next":
        case "skip-forward": return "\ue164";
        case "shuffle": return "\ue223";
        case "refresh":
        case "refresh-cw": return "\ue5b2";
        case "search": return "\ue19a";
        case "filter": return "\ue367";
        case "check": return "\ue070";
        case "database": return "\ue38a";
        case "sliders":
        case "sliders-horizontal": return "\ue29a";
        case "folder": return "\ue0dc";
        case "clock": return "\ue33a";
        case "moon": return "\ue135";
        case "image": return "\ue122";
        case "chevron-down": return "\ue071";
        case "right":
        case "chevron-right": return "\ue073";
        case "left":
        case "chevron-left": return "\ue072";
        case "info": return "\ue0fe";
        case "power": return "\ue376";
        case "star": return "\ue17a";
        case "server": return "\ue157";
        case "lock": return "\ue10f";
        case "volume":
        case "volume-2": return "\ue1ab";
        case "contrast": return "\ue0a1";
        case "plug": return "\ue383";
        case "eye": return "\ue0be";
        case "x":
        case "close": return "\ue1b2";
        case "palette": return "\ue1dd";
        case "globe": return "\ue0eb";
        case "circle-check": return "\ue226";
        case "circle-alert": return "\ue07b";
        case "triangle-alert": return "\ue193";
        default: return "\ue261";
        }
    }
}
