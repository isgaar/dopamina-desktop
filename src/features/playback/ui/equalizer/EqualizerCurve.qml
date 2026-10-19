/**
 * Location: src/features/playback/ui/equalizer/EqualizerCurve.qml
 * Function: Stellio frequency curve visualizer with curved card frame,
 * cubic spline interpolation, gradient fill, and active frequency nodes.
 * All Rights Reserved isgaar
 */
import QtQuick
import "root:/src/shared"

Rectangle {
    id: root
    height: 74; radius: Theme.radiusMd; color: Theme.bgSurface
    border.color: Theme.borderDark; border.width: 1; clip: true

    property var gains: equalizerController.bandGains
    property real preamp: equalizerController.preamp

    Canvas {
        id: canvas; anchors.fill: parent; anchors.margins: 6

        Connections {
            target: equalizerController
            function onParamsChanged() { canvas.requestPaint() }
        }

        onPaint: {
            var ctx = getContext("2d"), w = width, h = height
            ctx.clearRect(0, 0, w, h)
            var midY = h / 2.0

            ctx.beginPath(); ctx.setLineDash([4, 4])
            ctx.strokeStyle = Theme.borderDark; ctx.lineWidth = 1
            ctx.moveTo(0, midY); ctx.lineTo(w, midY); ctx.stroke()
            ctx.setLineDash([])

            var count = root.gains ? root.gains.length : 12
            var step = w / Math.max(1, count - 1), pts = []
            for (var i = 0; i < count; ++i) {
                var g = (root.gains && i < root.gains.length) ?
                    root.gains[i] : 0.0
                var total = g + root.preamp
                var y = midY - (total / 15.0) * (midY - 6.0)
                pts.push({ x: i * step, y: Math.max(4, Math.min(h - 4, y)) })
            }
            if (pts.length < 2) return

            // Gradient Area Fill under Curve
            ctx.beginPath(); ctx.moveTo(pts[0].x, midY)
            ctx.lineTo(pts[0].x, pts[0].y)
            for (var j = 0; j < pts.length - 1; ++j) {
                var cx = (pts[j].x + pts[j + 1].x) / 2.0
                ctx.bezierCurveTo(cx, pts[j].y, cx, pts[j + 1].y,
                                  pts[j + 1].x, pts[j + 1].y)
            }
            ctx.lineTo(pts[pts.length - 1].x, midY); ctx.closePath()
            var grad = ctx.createLinearGradient(0, 0, 0, h)
            grad.addColorStop(0, Theme.withAlpha(Theme.color("eq_coral"), 0.3))
            grad.addColorStop(1, Theme.withAlpha(Theme.color("eq_coral"), 0.02))
            ctx.fillStyle = grad; ctx.fill()

            // Curve stroke line
            ctx.beginPath(); ctx.moveTo(pts[0].x, pts[0].y)
            for (var k = 0; k < pts.length - 1; ++k) {
                var cpX = (pts[k].x + pts[k + 1].x) / 2.0
                ctx.bezierCurveTo(cpX, pts[k].y, cpX, pts[k + 1].y,
                                  pts[k + 1].x, pts[k + 1].y)
            }
            ctx.strokeStyle = Theme.color("eq_coral")
            ctx.lineWidth = 2.0; ctx.stroke()

            // Frequency nodes
            ctx.fillStyle = Theme.color("eq_coral")
            for (var n = 0; n < pts.length; ++n) {
                ctx.beginPath(); ctx.arc(pts[n].x, pts[n].y, 2, 0, 2 * Math.PI)
                ctx.fill()
            }
        }
    }
}
