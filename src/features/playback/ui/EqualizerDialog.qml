/**
 * Location: src/features/playback/ui/EqualizerDialog.qml
 * Function: Stellio-grade 12-band equalizer window with curved chrome,
 * animated segmented tabs, presets, and smooth audio DSP controls.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui/window"
import "root:/src/features/playback/ui/equalizer"

RoundedWindow {
    id: root
    width: 440; height: 720; minimumWidth: 400; minimumHeight: 600
    title: Strings.get("eq_title"); headerTitle: title
    property int currentTab: 0

    ColumnLayout {
        anchors.fill: parent; anchors.margins: Theme.spaceMd
        spacing: Theme.spaceSm

        EqualizerHeader { Layout.fillWidth: true }

        Rectangle {
            id: tabContainer
            Layout.fillWidth: true; Layout.preferredHeight: 36
            radius: Theme.radiusFull; color: Theme.bgSurface
            border.color: Theme.borderDark; border.width: 1

            property var tabLabels: [
                Strings.get("eq_tab_bands"),
                Strings.get("eq_tab_effects1"),
                Strings.get("eq_tab_effects2")
            ]
            property real tabW: (width - 4) / 3

            Rectangle {
                x: 2 + root.currentTab * tabContainer.tabW
                y: 2; width: tabContainer.tabW; height: tabContainer.height - 4
                radius: Theme.radiusFull; color: Theme.color("eq_coral")

                Behavior on x {
                    SpringAnimation { spring: 3.0; damping: 1.0; epsilon: 0.25 }
                }
            }

            Row {
                anchors.fill: parent; anchors.margins: 2
                Repeater {
                    model: tabContainer.tabLabels
                    Item {
                        width: tabContainer.tabW
                        height: tabContainer.height - 4
                        Text {
                            anchors.centerIn: parent; text: modelData
                            font.pixelSize: 12; font.bold: true
                            font.family: Theme.fontFamily
                            color: root.currentTab === index ?
                                Theme.bgDarkest : Theme.textMuted
                            Behavior on color {
                                ColorAnimation { duration: 150 }
                            }
                        }
                        MouseArea {
                            anchors.fill: parent
                            cursorShape: Qt.PointingHandCursor
                            onClicked: root.currentTab = index
                        }
                    }
                }
            }
        }

        Rectangle {
            Layout.fillWidth: true; Layout.fillHeight: true
            radius: Theme.radiusMd; color: Theme.bgSurface
            border.color: Theme.borderDark; border.width: 1; clip: true

            ScrollView {
                anchors.fill: parent; anchors.margins: Theme.spaceSm; clip: true
                ScrollBar.horizontal.policy: ScrollBar.AlwaysOff
                ScrollBar.vertical.policy: ScrollBar.AsNeeded

                Loader {
                    id: tabLoader; width: parent.width
                    sourceComponent: {
                        if (root.currentTab === 0) return bandsComp
                        if (root.currentTab === 1) return fx1Comp
                        return fx2Comp
                    }
                }
            }
        }
    }

    Component { id: bandsComp; EqualizerBandsTab {} }
    Component { id: fx1Comp; EqualizerEffects1Tab {} }
    Component { id: fx2Comp; EqualizerEffects2Tab {} }
}
