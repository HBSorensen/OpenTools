import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page
    property var controller

    // Manual control is disabled while a program streams.
    readonly property bool canJog: controller.connected && !controller.running
    readonly property real step: [0.1, 1.0, 10.0][stepGroup.selected]
    readonly property real jogFeed: 1000

    // Reusable jog button (declared at the root so its scope is well-defined).
    component JogButton: Button {
        Layout.preferredWidth: 84
        Layout.preferredHeight: 62
        font.pixelSize: 22
        enabled: page.canJog
    }
    component JogSpacer: Item {
        Layout.preferredWidth: 84
        Layout.preferredHeight: 62
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 16

        // --- XY jog pad ---
        GridLayout {
            columns: 3
            rowSpacing: 8
            columnSpacing: 8
            Layout.alignment: Qt.AlignVCenter

            JogSpacer {}
            JogButton { text: "Y+"; onClicked: controller.jog("Y",  page.step, page.jogFeed) }
            JogSpacer {}

            JogButton { text: "X-"; onClicked: controller.jog("X", -page.step, page.jogFeed) }
            JogButton { text: "⌂"; onClicked: controller.home() } // house glyph = home
            JogButton { text: "X+"; onClicked: controller.jog("X",  page.step, page.jogFeed) }

            JogSpacer {}
            JogButton { text: "Y-"; onClicked: controller.jog("Y", -page.step, page.jogFeed) }
            JogSpacer {}
        }

        // --- Z ---
        ColumnLayout {
            spacing: 8
            JogButton { text: "Z+"; onClicked: controller.jog("Z",  page.step, page.jogFeed) }
            JogButton { text: "Z-"; onClicked: controller.jog("Z", -page.step, page.jogFeed) }
        }

        // --- Step size + machine actions ---
        ColumnLayout {
            Layout.fillWidth: true
            spacing: 8

            Label { text: qsTr("Step (mm)"); font.pixelSize: 16 }
            RowLayout {
                id: stepGroup
                property int selected: 1
                spacing: 6
                Repeater {
                    model: ["0.1", "1", "10"]
                    Button {
                        text: modelData
                        Layout.fillWidth: true
                        Layout.preferredHeight: 52
                        font.pixelSize: 18
                        checkable: true
                        checked: stepGroup.selected === index
                        onClicked: stepGroup.selected = index
                    }
                }
            }

            Item { Layout.preferredHeight: 8 }

            Button {
                text: qsTr("Zero work (XYZ)")
                Layout.fillWidth: true; Layout.preferredHeight: 52
                font.pixelSize: 18
                enabled: page.canJog
                onClicked: controller.zeroWork()
            }
            Button {
                text: qsTr("Unlock ($X)")
                Layout.fillWidth: true; Layout.preferredHeight: 52
                font.pixelSize: 18
                enabled: controller.connected
                onClicked: controller.unlock()
            }
            Button {
                text: qsTr("Soft reset")
                Layout.fillWidth: true; Layout.preferredHeight: 52
                font.pixelSize: 18
                enabled: controller.connected
                onClicked: controller.softReset()
            }
        }
    }
}
