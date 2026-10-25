/**
 * Location: src/Main.qml
 * Function: Root application window hosting custom title bar, SideNav rail,
 * onboarding flow, library, playlists, queue, and the playback bar.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Effects
import QtQuick.Window
import "shared/ui"
import "shared/ui/navigation"
import "shared/ui/titlebar"
import "shared/ui/window"
import "features/playback/ui"
import "features/session/ui"
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
    readonly property int dashH: Math.round(800 * dp)
    property string activeView: "tracks"
    width: isSetup ? setupW : dashW; height: isSetup ? setupH : dashH
    minimumWidth: isSetup ? Math.round(440 * dp) : Math.round(960 * dp)
    minimumHeight: isSetup ? Math.round(340 * dp) : Math.round(600 * dp)
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
        anchors.fill: parent; color: Theme.bgDarkest
        radius: isFloat ? Theme.radiusMd : 0
        border.color: isFloat ? Theme.borderDark : "transparent"
        border.width: isFloat ? 1 : 0; layer.enabled: window.isFloat
        layer.effect: MultiEffect { maskEnabled: true; maskSource: cornerMask }

        AppTitleBar {
            id: titleBar; anchors.top: parent.top; window: window
            anchors.left: parent.left; anchors.right: parent.right
            isFirstRun: window.isSetup
            onOpenSessionRequested: sessionWindow.showWindow()
            onSearchEdited: text => {
                window.activeView = "tracks"
                if (libraryController) libraryController.search(text)
            }
        }

        FirstRunWizardDialog {
            anchors.top: titleBar.bottom; anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            anchors.margins: Theme.spaceSm; anchors.topMargin: 0
            visible: window.isSetup
        }

        Loader {
            anchors.top: titleBar.bottom; anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            active: !window.isSetup
            sourceComponent: Component {
                MainDashboard {
                    activeView: window.activeView
                    onActiveViewChanged: window.activeView = activeView
                    onOpenSettingsRequested: settingsWindow.showSettings()
                    onOpenEqualizerRequested: equalizerWindow.showWindow()
                    onOpenTimerRequested: sleepTimerDialog.showWindow()
                    onOpenAboutRequested: aboutWindow.showWindow()
                }
            }
        }
    }

    RoundedMask { id: cornerMask; anchors.fill: windowRoot
        cornerRadius: window.isFloat ? Theme.radiusMd : 0 }

    WindowResizeHandles {
        anchors.fill: parent
        window: window
    }

    Connections { target: settingsController
        function onFirstRunChanged() { window.syncWindowSize() }
    }

    AboutWindow { id: aboutWindow }
    SettingsWindow { id: settingsWindow; objectName: "settingsWindow" }
    SessionWindow { id: sessionWindow }
    EqualizerDialog { id: equalizerWindow }
    SleepTimerDialog { id: sleepTimerDialog }
}
