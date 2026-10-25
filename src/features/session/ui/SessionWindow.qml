/**
 * Location: src/features/session/ui/SessionWindow.qml
 * Function: Frameless account window hosting the session screen and reacting
 * to web handoff requests with an unobtrusive desktop presentation.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import "root:/src/shared"
import "root:/src/shared/ui/window"

RoundedWindow {
    id: root
    objectName: "sessionWindow"
    width: 460; height: 600
    minimumWidth: 420; minimumHeight: 540
    maximumWidth: 560; maximumHeight: 760
    title: Strings.get("session_title")
    headerTitle: title

    SessionView { anchors.fill: parent }

    Connections {
        target: sessionController
        function onSessionWindowRequested() { root.showWindow() }
    }

    Shortcut {
        sequence: "Escape"
        onActivated: root.close()
    }
}
