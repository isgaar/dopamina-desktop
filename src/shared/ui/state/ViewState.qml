/**
 * Location: src/shared/ui/state/ViewState.qml
 * Function: Master view state container switching between loading, content,
 * stale, empty, offline, error, and forbidden states with distinct visual copy.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"
import "root:/src/shared/ui/feedback"

Item {
    id: root

    // "loading", "content", "stale", "empty", "offline", "error", "forbidden"
    property string stateMode: "content"
    property string errorCode: ""
    property string requestId: ""
    property string customMessage: ""
    default property alias content: contentHost.data

    signal retryRequested()
    signal editServerRequested()
    signal loginRequested()

    Item {
        id: contentHost
        anchors.fill: parent
        visible: root.stateMode === "content" || root.stateMode === "stale"
    }

    Rectangle {
        id: staleBanner
        anchors.top: parent.top; anchors.left: parent.left
        anchors.right: parent.right; height: 32
        visible: root.stateMode === "stale"
        color: Theme.accentOrange; radius: Theme.radiusSm
        RowLayout {
            anchors.fill: parent; anchors.margins: Theme.spaceSm
            Text {
                text: Strings.get("state_stale_banner")
                color: Theme.bgDark; font.bold: true
                font.family: Theme.fontFamily; Layout.fillWidth: true
            }
            AppButton {
                text: Strings.get("btn_retry")
                onClicked: root.retryRequested()
            }
        }
    }

    EmptyStateView {
        anchors.centerIn: parent
        visible: root.stateMode === "loading"
        isLoading: true
        title: Strings.get("state_loading_title")
        description: ""
    }

    EmptyStateView {
        anchors.centerIn: parent
        visible: root.stateMode === "empty"
        iconName: "folder-open"
        title: Strings.get("empty_section_title")
        description: Strings.get("empty_section_desc")
    }

    EmptyStateView {
        anchors.centerIn: parent
        visible: root.stateMode === "offline"
        iconName: "wifi-off"
        title: Strings.get("state_offline_title")
        description: Strings.get("err_conn_timeout")
        showAction: true
        actionText: Strings.get("btn_retry")
        onActionTriggered: root.retryRequested()
    }

    EmptyStateView {
        anchors.centerIn: parent
        visible: root.stateMode === "error"
        iconName: "alert-triangle"
        title: Strings.get("state_error_title")
        description: root.customMessage.length > 0
            ? root.customMessage : root.errorCode
        showAction: true
        actionText: Strings.get("btn_retry")
        onActionTriggered: root.retryRequested()
    }

    EmptyStateView {
        anchors.centerIn: parent
        visible: root.stateMode === "forbidden"
        iconName: "lock"
        title: Strings.get("state_forbidden_title")
        description: Strings.get("err_auth_bad_credentials")
        showAction: true
        actionText: Strings.get("btn_login")
        onActionTriggered: root.loginRequested()
    }
}
