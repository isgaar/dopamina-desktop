/**
 * Location: src/shared/ui/SegmentedControl.qml
 * Function: Multi-option segmented button control providing distinct visual
 * segments, active selection pill styling, and keyboard mouse interactions.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"

ColumnLayout {
    id: root
    property string label: ""
    property var options: []
    property int currentIndex: 0
    signal activated(int index)

    Layout.fillWidth: true
    spacing: Theme.spaceXs

    Text {
        text: root.label
        font.family: Theme.fontFamily
        font.pixelSize: 12
        font.bold: true
        color: Theme.textLight
        visible: root.label.length > 0
    }

    Rectangle {
        Layout.fillWidth: true
        Layout.preferredHeight: 32
        radius: Theme.radiusSm
        color: Theme.bgSurface
        border.color: Theme.borderDark
        border.width: 1

        RowLayout {
            anchors.fill: parent
            anchors.margins: 2
            spacing: 2

            Repeater {
                model: root.options
                delegate: Rectangle {
                    id: segItem
                    readonly property bool isSelected:
                        index === root.currentIndex
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    radius: Theme.radiusSm
                    color: isSelected ? Theme.bgCard : "transparent"
                    border.color: isSelected ? Theme.borderDark : "transparent"
                    border.width: isSelected ? 1 : 0

                    Text {
                        anchors.centerIn: parent
                        text: modelData
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        font.bold: segItem.isSelected
                        color: segItem.isSelected ? Theme.primary :
                               Theme.textMuted
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            root.currentIndex = index
                            root.activated(index)
                        }
                    }
                }
            }
        }
    }
}
