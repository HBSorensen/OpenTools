import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    property var controller

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 10

        RowLayout {
            Layout.fillWidth: true
            spacing: 10

            Label { text: qsTr("Port:"); font.pixelSize: 18 }

            ComboBox {
                id: portBox
                Layout.fillWidth: true
                Layout.preferredHeight: 52
                font.pixelSize: 18
                model: controller.availablePorts
                enabled: !controller.connected
            }

            Button {
                text: qsTr("Rescan")
                Layout.preferredHeight: 52
                font.pixelSize: 18
                enabled: !controller.connected
                onClicked: controller.refreshPorts()
            }

            Button {
                text: controller.connected ? qsTr("Disconnect") : qsTr("Connect")
                Layout.preferredHeight: 52
                Layout.preferredWidth: 150
                font.pixelSize: 18
                highlighted: true
                enabled: controller.connected || portBox.currentText !== ""
                onClicked: {
                    if (controller.connected)
                        controller.close()
                    else
                        controller.open(portBox.currentText, 115200)
                }
            }
        }

        Label {
            text: qsTr("115200 baud · 8-N-1 · grblHAL over USB serial")
            font.pixelSize: 14
            color: "#757575"
        }

        // Live controller console.
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 4

            ScrollView {
                anchors.fill: parent
                clip: true

                TextArea {
                    id: console
                    readOnly: true
                    wrapMode: TextArea.NoWrap
                    font.family: "monospace"
                    font.pixelSize: 14
                    text: controller.console
                    onTextChanged: cursorPosition = length // auto-scroll to bottom
                }
            }
        }
    }
}
