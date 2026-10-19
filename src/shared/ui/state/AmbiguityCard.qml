/**
 * Location: src/shared/ui/state/AmbiguityCard.qml
 * Function: Three-track contrast question card for user genre resolution.
 * All Rights Reserved isgaar
 */
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import "root:/src/shared"
import "root:/src/shared/ui"

Rectangle {
    id: root
    property string seedTitle: ""
    property var options: []
    property bool applyToAlbum: false

    signal optionChosen(int index, string station, bool applyAlbum)
    signal chooseNone()
    signal deferQuestion()
    signal previewRequested(int index)

    color: Theme.bgCard
    border.color: Theme.borderDark
    border.width: 1
    radius: Theme.radiusMd
    implicitHeight: layout.implicitHeight + 24

    ColumnLayout {
        id: layout
        anchors.fill: parent
        anchors.margins: 12
        spacing: Theme.spaceSm

        Text {
            text: Strings.get("mix_ambiguity_title").arg(root.seedTitle)
            font.family: Theme.fontFamily
            font.pixelSize: 13
            font.bold: true
            color: Theme.textLight
            Layout.fillWidth: true
            elide: Text.ElideRight
        }

        Repeater {
            model: root.options
            delegate: Rectangle {
                Layout.fillWidth: true
                height: 36
                color: Theme.bgSurface
                radius: Theme.radiusSm
                border.color: Theme.borderDark
                border.width: 1

                RowLayout {
                    anchors.fill: parent
                    anchors.margins: 6
                    spacing: Theme.spaceSm

                    AppButton {
                        Layout.preferredWidth: 26
                        Layout.preferredHeight: 26
                        iconName: "play"
                        variant: "ghost"
                        onClicked: root.previewRequested(index)
                    }

                    Text {
                        text: (modelData.titulo || "") + " · " +
                              (modelData.artista || "")
                        font.family: Theme.fontFamily
                        font.pixelSize: 12
                        color: Theme.textLight
                        Layout.fillWidth: true
                        elide: Text.ElideRight
                    }

                    AppButton {
                        Layout.preferredHeight: 26
                        text: Strings.get("mix_ambiguity_preview")
                        variant: "secondary"
                        onClicked: root.optionChosen(
                            index, modelData.estacion || "", root.applyToAlbum)
                    }
                }
            }
        }

        RowLayout {
            Layout.fillWidth: true
            spacing: Theme.spaceSm

            CheckBox {
                id: albumCheck
                text: Strings.get("mix_ambiguity_apply_album")
                checked: root.applyToAlbum
                onToggled: root.applyToAlbum = checked
            }

            Item { Layout.fillWidth: true }

            AppButton {
                text: Strings.get("mix_ambiguity_none")
                variant: "secondary"
                onClicked: root.chooseNone()
            }

            AppButton {
                text: Strings.get("mix_ambiguity_later")
                variant: "secondary"
                onClicked: root.deferQuestion()
            }
        }
    }
}
