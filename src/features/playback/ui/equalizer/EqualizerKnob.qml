/**
 * Location: src/features/playback/ui/equalizer/EqualizerKnob.qml
 * Function: Rotary dial with curved card frame and power toggle.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

Rectangle { id: root
    width: 120; height: 126; radius: Theme.radiusMd; color: Theme.bgCard
    border.color: knobMouse.containsMouse ?
        Theme.borderHighlight : Theme.borderDark
    border.width: 1; clip: true
    Behavior on border.color { ColorAnimation { duration: 120 } }

    property string title: ""
    property real value: 0; property bool active: false
    signal valueChangedByUser(real val); signal toggledByUser(bool en)

    ColumnLayout {
        anchors.centerIn: parent; spacing: 4
        Item {
            Layout.alignment: Qt.AlignHCenter; Layout.preferredWidth: 72
            Layout.preferredHeight: 72
            Canvas {
                id: knobCanvas; anchors.fill: parent
                onPaint: {
                    var c = getContext("2d"), w = width, h = height
                    var r = w / 2 - 6, s = 0.75 * Math.PI, e = 2.25 * Math.PI
                    c.clearRect(0, 0, w, h); c.lineCap = "round"
                    c.beginPath(); c.arc(w / 2, h / 2, r, s, e)
                    c.strokeStyle = Theme.bgSelected; c.lineWidth = 5
                    c.stroke()
                    var p = Math.max(0.0, Math.min(1.0, root.value / 100.0))
                    if (p <= 0.0) return
                    c.beginPath(); c.arc(w / 2, h / 2, r, s, s + p * (e - s))
                    c.strokeStyle = root.active ?
                        Theme.color("eq_coral") : Theme.borderHighlight
                    c.lineWidth = 5; c.stroke()
                }
            }
            Text {
                anchors.centerIn: parent; text: Math.round(root.value)
                font.pixelSize: 16; font.bold: true
                font.family: Theme.fontFamily
                color: root.active ? Theme.color("eq_coral") : Theme.textLight
            }
            MouseArea {
                id: knobMouse; anchors.fill: parent; hoverEnabled: true
                cursorShape: Qt.PointingHandCursor; property real lastY: 0
                onPressed: mouse => lastY = mouse.y
                onPositionChanged: mouse => {
                    if (!pressed) return
                    var d = lastY - mouse.y; lastY = mouse.y
                    root.valueChangedByUser(
                        Math.max(0, Math.min(100, root.value + d)))
                    knobCanvas.requestPaint()
                }
            }
        }
        RowLayout {
            Layout.alignment: Qt.AlignHCenter; spacing: 4
            AppButton {
                Layout.preferredWidth: 20; Layout.preferredHeight: 20
                iconName: "power"
                variant: root.active ? "primary" : "secondary"
                onClicked: root.toggledByUser(!root.active)
            }
            Text {
                text: root.title; font.pixelSize: 11
                font.family: Theme.fontFamily; elide: Text.ElideRight
                color: root.active ? Theme.color("eq_coral") : Theme.textMuted
            }
        }
    }
    onValueChanged: knobCanvas.requestPaint()
    onActiveChanged: knobCanvas.requestPaint()
}
