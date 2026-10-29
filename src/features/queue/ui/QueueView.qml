/**
 * Location: src/features/queue/ui/QueueView.qml
 * Function: Queue drawer showing queued tracks, current playing item indicator,
 * mix seed title badge, operational toggles, and empty state feedback.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "../../../shared/ui"
import "../../../shared/ui/feedback"
import "../../library/ui"

Rectangle {
    id: root
    color: Theme.bgCard
    radius: Theme.radiusMd; clip: true
    border.color: Theme.borderDark; border.width: 1

    property var controller: null
    property var playbackCtrl: null

    ColumnLayout {
        anchors.fill: parent; anchors.margins: Theme.spaceMd
        spacing: Theme.spaceSm

        RowLayout {
            Layout.fillWidth: true; spacing: Theme.spaceMd

            Rectangle {
                visible: root.controller && root.controller.isMixActive
                height: 24; width: mixText.implicitWidth + 16
                radius: Theme.radiusSm; color: Theme.primaryDim

                Text {
                    id: mixText; anchors.centerIn: parent
                    text: root.controller ?
                        Strings.get("label_mix_of").replace("%1",
                            root.controller.mixSeedTitle) : ""
                    font.family: Theme.fontFamily
                    font.pixelSize: Theme.fontSm; color: Theme.primaryHover
                }
            }

            Item { Layout.fillWidth: true }

            AppButton {
                text: Strings.get("btn_clear")
                onClicked: if (root.controller) root.controller.clearQueue()
            }
        }

        ListView {
            id: listView
            Layout.fillWidth: true; Layout.fillHeight: true; clip: true
            visible: count > 0
            model: root.controller ? root.controller.queueTracks : []

            delegate: Rectangle {
                id: queueItemRect
                width: listView.width; height: 48; radius: Theme.radiusSm
                color: (root.controller &&
                    index === root.controller.currentIndex) ?
                    Theme.bgSurface : (itemHover.containsMouse ? "#181822" : "transparent")

                MouseArea {
                    id: itemHover
                    anchors.fill: parent
                    hoverEnabled: true
                    acceptedButtons: Qt.LeftButton | Qt.RightButton
                    cursorShape: Qt.PointingHandCursor
                    onClicked: mouse => {
                        if (mouse.button === Qt.RightButton) {
                            trackMenu.openAtItem(queueItemRect, mouse.x, mouse.y)
                        } else if (root.controller) {
                            root.controller.playAtIndex(index)
                        }
                    }
                }

                RowLayout {
                    anchors.fill: parent; anchors.margins: Theme.spaceSm
                    spacing: Theme.spaceSm

                    Text {
                        text: (index + 1) + "."
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontSm; color: Theme.textMuted
                        Layout.preferredWidth: 24
                    }

                    Rectangle {
                        Layout.preferredWidth: 32; Layout.preferredHeight: 32
                        radius: Theme.radiusSm; color: Theme.bgCard
                        border.color: Theme.borderDark; border.width: 1

                        AppIcon {
                            anchors.centerIn: parent
                            name: "disc"; size: 14; iconColor: Theme.textDim
                        }
                        RoundedImage {
                            anchors.fill: parent; radius: Theme.radiusSm
                            source: modelData.coverUrl || modelData.cover || ""
                        }
                    }

                    Text {
                        text: modelData.title || modelData.titulo ||
                            (Strings.get("tab_tracks") + " #" + modelData.id)
                        font.family: Theme.fontFamily
                        font.pixelSize: Theme.fontMd; color: Theme.textLight
                        Layout.fillWidth: true; elide: Text.ElideRight
                    }

                    AudioQualityBadge {
                        track: modelData
                    }

                    AppIcon {
                        visible: modelData.isMix === true
                        name: "mix"; size: 16
                        iconColor: Theme.primaryHover
                    }
                }

                TrackContextMenu {
                    id: trackMenu
                    track: modelData
                    isLiked: false
                }
            }
        }

        EmptyStateView {
            Layout.fillWidth: true; Layout.fillHeight: true
            visible: listView.count === 0
            iconName: "disc"
            showAction: false
        }
    }
}
