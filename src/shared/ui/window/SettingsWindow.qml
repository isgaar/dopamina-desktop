/**
 * Location: src/shared/ui/window/SettingsWindow.qml
 * Function: Desktop preferences window with responsive category sidebar,
 * draft transaction lifecycle, dirty state tracking, and in-window dialogs.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/settings"
import "root:/src/features/settings/ui"
import "root:/src/features/settings/ui/panes"
RoundedWindow {
    id: root; objectName: "settingsWindow"
    width: 760; height: 480; minimumWidth: 640; minimumHeight: 420
    maximumWidth: 1200; maximumHeight: 800
    title: Strings.get("settings_title") + Theme.appName; headerTitle: title
    readonly property bool isNarrow: width < 640
    property bool showConfirm: false

    function showSettings() {
        if (settingsController) settingsController.resetDrafts()
        showConfirm = false; root.showWindow()
    }
    function saveSettings() {
        if (settingsController) settingsController.apply()
        if (stationController) stationController.reload()
        root.close()
    }
    onClosing: function(e) {
        if (settingsController && settingsController.dirty) {
            e.accepted = false; showConfirm = true
        }
    }
    Item {
        anchors.fill: parent

        SettingsNav {
            id: nav; horizontal: root.isNarrow
            anchors.top: parent.top; anchors.left: parent.left
            anchors.right: root.isNarrow ? parent.right : undefined
            anchors.bottom: root.isNarrow ? undefined : footer.top
            currentIndex: stack.currentIndex
            categories: [
                { id: "audio", title: Strings.get("settings_nav_audio"),
                  icon: "sliders" },
                { id: "source", title: Strings.get("settings_nav_source"),
                  icon: "folder" },
                { id: "covers", title: Strings.get("settings_nav_covers"),
                  icon: "image" },
                { id: "appearance", icon: "contrast",
                  title: Strings.get("settings_nav_appearance") },
                { id: "system", title: Strings.get("settings_nav_system"),
                  icon: "settings" }
            ]
            onCategorySelected: idx => stack.currentIndex = idx
        }

        Rectangle {
            id: divV; color: Theme.borderDark; visible: !root.isNarrow
            anchors.top: parent.top; anchors.bottom: footer.top
            anchors.left: nav.right; width: 1
        }
        Rectangle {
            id: divH; color: Theme.borderDark; visible: root.isNarrow
            anchors.top: nav.bottom; anchors.left: parent.left
            anchors.right: parent.right; height: 1
        }

        StackLayout {
            id: stack; objectName: "settingsStack"
            anchors.top: root.isNarrow ? divH.bottom : parent.top
            anchors.left: root.isNarrow ? parent.left : divV.right
            anchors.right: parent.right; anchors.bottom: footer.top
            AudioSettingsPane {}
            MusicSourceSettingsPane {}
            CoversSettingsPane {}
            AppearanceSettingsPane {}
            SystemSettingsPane {}
        }

        SettingsFooter {
            id: footer; anchors.bottom: parent.bottom
            anchors.left: parent.left; anchors.right: parent.right
            isDirty: settingsController ? settingsController.dirty : false
            onCloseClicked: isDirty ? (showConfirm = true) : root.close()
            onDiscardClicked: if (settingsController)
                settingsController.discard()
            onSaveClicked: root.saveSettings()
        }
        SettingsConfirmDialog {
            anchors.fill: parent; visible: root.showConfirm
            onKeepEditingRequested: root.showConfirm = false
            onDiscardRequested: {
                if (settingsController) settingsController.discard()
                showConfirm = false; root.close()
            }
            onSaveRequested: { root.saveSettings(); showConfirm = false }
        }
    }
    Shortcut {
        sequence: "Escape"
        onActivated: showConfirm ? (showConfirm = false) :
            ((settingsController && settingsController.dirty) ?
                (showConfirm = true) : root.close())
    }
}
