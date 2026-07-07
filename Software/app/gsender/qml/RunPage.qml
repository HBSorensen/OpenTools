import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page
    property var controller

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 12

        Label {
            Layout.fillWidth: true
            text: controller.programName !== ""
                  ? controller.programName
                  : qsTr("No program loaded — pick one on the Files tab")
            font.pixelSize: 20; font.bold: true
            elide: Text.ElideMiddle
        }

        Label {
            Layout.fillWidth: true
            visible: controller.autoLevel && controller.haveHeightMap
            text: qsTr("⤳ Auto-level ON (%1×%2 map)")
                    .arg(controller.mapCols).arg(controller.mapRows)
            font.pixelSize: 14; color: "#1565c0"
        }

        // Progress.
        ProgressBar {
            Layout.fillWidth: true
            Layout.preferredHeight: 26
            from: 0; to: 1
            value: controller.progress
        }
        Label {
            text: Math.round(controller.progress * 100) + "%  ("
                  + Math.round(controller.progress * controller.programLines)
                  + " / " + controller.programLines + " lines)"
            font.pixelSize: 16; color: "#555"
        }

        // Transport controls.
        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Button {
                text: qsTr("Start")
                Layout.fillWidth: true; Layout.preferredHeight: 68
                font.pixelSize: 20; highlighted: true
                enabled: controller.connected && controller.programLines > 0
                         && !controller.running && !controller.probing
                onClicked: controller.startProgram()
            }
            Button {
                text: qsTr("Hold")
                Layout.fillWidth: true; Layout.preferredHeight: 68
                font.pixelSize: 20
                enabled: controller.running
                onClicked: controller.feedHold()
            }
            Button {
                text: qsTr("Resume")
                Layout.fillWidth: true; Layout.preferredHeight: 68
                font.pixelSize: 20
                enabled: controller.connected
                onClicked: controller.resume()
            }
            Button {
                text: qsTr("Stop")
                Layout.fillWidth: true; Layout.preferredHeight: 68
                font.pixelSize: 20
                enabled: controller.connected
                onClicked: controller.stopProgram()
            }
        }

        // Feed override.
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            Label { text: qsTr("Feed"); font.pixelSize: 18 }
            Button {
                text: "−10%"; Layout.preferredHeight: 52; font.pixelSize: 16
                enabled: controller.connected
                onClicked: controller.setFeedOverride(controller.feedOverride - 10)
            }
            Label {
                Layout.preferredWidth: 80
                horizontalAlignment: Text.AlignHCenter
                text: controller.feedOverride + "%"
                font.pixelSize: 20; font.bold: true
            }
            Button {
                text: "+10%"; Layout.preferredHeight: 52; font.pixelSize: 16
                enabled: controller.connected
                onClicked: controller.setFeedOverride(controller.feedOverride + 10)
            }
            Button {
                text: qsTr("100%"); Layout.preferredHeight: 52; font.pixelSize: 16
                enabled: controller.connected
                onClicked: controller.setFeedOverride(100)
            }
            Item { Layout.fillWidth: true }
            Label {
                text: qsTr("Feed: %1").arg(controller.feed.toFixed(0))
                font.pixelSize: 16; color: "#555"
            }
        }

        Item { Layout.fillHeight: true }
    }
}
