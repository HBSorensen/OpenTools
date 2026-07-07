import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

// Digital readout header: connection dot, machine state, work-coordinate X/Y/Z.
ToolBar {
    id: root
    property var controller

    RowLayout {
        anchors.fill: parent
        anchors.leftMargin: 12
        anchors.rightMargin: 12
        spacing: 16

        // Connection indicator.
        Rectangle {
            width: 16; height: 16; radius: 8
            color: controller.connected ? "#2e7d32" : "#9e9e9e"
        }

        // Machine state.
        Label {
            Layout.preferredWidth: 90
            text: controller.state
            font.pixelSize: 22
            font.bold: true
            color: {
                switch (controller.state) {
                case "Idle":  return "#2e7d32";
                case "Run":   return "#1565c0";
                case "Hold":  return "#ef6c00";
                case "Jog":   return "#1565c0";
                case "Alarm": return "#c62828";
                default:      return "#616161";
                }
            }
        }

        Item { Layout.fillWidth: true }

        // Work-coordinate position.
        Repeater {
            model: 3
            RowLayout {
                spacing: 4
                Label {
                    text: ["X", "Y", "Z"][index]
                    font.pixelSize: 20; font.bold: true; color: "#555"
                }
                Label {
                    Layout.preferredWidth: 96
                    horizontalAlignment: Text.AlignRight
                    font.pixelSize: 22
                    font.family: "monospace"
                    text: (index === 0 ? controller.wx
                          : index === 1 ? controller.wy
                          : controller.wz).toFixed(3)
                }
            }
        }
    }
}
