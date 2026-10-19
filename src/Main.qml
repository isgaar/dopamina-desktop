/**
 * Location: src/Main.qml
 * Function: Root application window hosting custom title bar, SideNav rail,
 * onboarding flow, library, playlists view, admin view, and PlayerBar.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Window
import "shared/ui"
import "shared/ui/navigation"
import "shared/ui/titlebar"
import "shared/ui/window"
import "features/playback/ui"
import "features/settings/ui"

ApplicationWindow {
    id: window
    readonly property real dp: Screen.logicalPixelDensity > 0 ?
        Screen.logicalPixelDensity / 3.78 :
        Math.max(1.0, Screen.devicePixelRatio)
    readonly property bool isFloat: Platform.useCustomTitleBar &&
        visibility !== Window.Maximized
    readonly property bool isSetup: settingsController ?
        settingsController.isFirstRun : false
    readonly property int setupW: Math.round(540 * dp)
    readonly property int setupH: Math.round(480 * dp)
    readonly property int dashW: Math.round(1240 * dp)
    readonly property int dashH: Math.round(780 * dp)
    property string activeView: "tracks"

    width: isSetup ? setupW : dashW; height: isSetup ? setupH : dashH
    minimumWidth: isSetup ? Math.round(440 * dp) : Math.round(980 * dp)
    minimumHeight: isSetup ? Math.round(340 * dp) : Math.round(620 * dp)
    visible: true; title: Theme.appName + " • " + Strings.get("app_tagline")
    color: "transparent"
    flags: Platform.useCustomTitleBar ?
        (Qt.Window | Qt.FramelessWindowHint) : Qt.Window

    function syncWindowSize() {
        if (!isSetup && visibility === Window.Maximized) return;
        var tw = isSetup ? setupW : dashW, th = isSetup ? setupH : dashH;
        x = Math.max(0, (Screen.desktopAvailableWidth - tw) / 2);
        y = Math.max(0, (Screen.desktopAvailableHeight - th) / 2);
        width = tw; height = th;
    }

    Component.onCompleted: syncWindowSize()

    Loader {
        active: Platform.useSystemMenuExport
        sourceComponent: Component { AppMenuBar {
            onOpenAboutRequested: aboutWindow.showWindow()
            onOpenSettingsRequested: settingsWindow.showSettings()
            onOpenEqualizerRequested: equalizerWindow.showWindow()
        }}
    }

    Rectangle {
        id: windowRoot
        anchors.fill: parent; color: Theme.bgSurface; clip: true
        radius: isFloat ? Theme.radiusMd : 0
        border.color: isFloat ? Theme.borderDark : "transparent"
        border.width: 1

        AppTitleBar {
            id: titleBar; anchors.top: parent.top; window: window
            anchors.left: parent.left; anchors.right: parent.right
            isFirstRun: window.isSetup; activeView: window.activeView
            onViewChangeRequested: v => window.activeView = v
            onOpenAboutRequested: aboutWindow.showWindow()
            onOpenSettingsRequested: settingsWindow.showSettings()
            onOpenEqualizerRequested: equalizerWindow.showWindow()
        }

        FirstRunWizardDialog {
            anchors.top: titleBar.bottom; anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            anchors.margins: Theme.spaceSm; anchors.topMargin: 0
            visible: window.isSetup
        }

        Loader {
            id: mainDashboardLoader
            anchors.top: titleBar.bottom; anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            active: !window.isSetup
            sourceComponent: Component {
                MainDashboard {
                    activeView: window.activeView
                    isFloating: window.isFloat
                    onActiveViewChanged: window.activeView = activeView
                    onOpenSettingsRequested: settingsWindow.showSettings()
                    onOpenEqualizerRequested: equalizerWindow.showWindow()
                    onOpenTimerRequested: sleepTimerDialog.showWindow()
                }
            }
        }
    }

    Connections {
        target: settingsController
        function onFirstRunChanged() { window.syncWindowSize() }
    }

    AboutWindow { id: aboutWindow }
    SettingsWindow { id: settingsWindow; objectName: "settingsWindow" }
    EqualizerDialog { id: equalizerWindow }
    SleepTimerDialog { id: sleepTimerDialog }
}
