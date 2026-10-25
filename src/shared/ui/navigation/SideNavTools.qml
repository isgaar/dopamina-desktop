/**
 * Location: src/shared/ui/navigation/SideNavTools.qml
 * Function: Bottom sidebar shortcuts for equalizer, timer, about, settings,
 * and the rail collapse toggle, stacked when icon-only.
 * All Rights Reserved isgaar
 */
import QtQuick
import ".."

Grid {
    id: root
    property bool collapsed: false
    spacing: 2
    columns: collapsed ? 1 : 5

    signal equalizerRequested()
    signal timerRequested()
    signal aboutRequested()
    signal settingsRequested()
    signal collapseRequested()

    readonly property var actions: [
        { i: "sliders", f: () => root.equalizerRequested() },
        { i: "moon", f: () => root.timerRequested() },
        { i: "info", f: () => root.aboutRequested() },
        { i: "settings", f: () => root.settingsRequested() },
        { i: collapsed ? "panel-right" : "panel-left",
          f: () => root.collapseRequested() }
    ]

    Repeater {
        model: root.actions
        delegate: AppButton {
            width: (root.width - (root.columns - 1) * root.spacing) /
                root.columns
            height: 30
            iconName: modelData.i
            variant: "ghost"
            onClicked: modelData.f()
        }
    }
}
