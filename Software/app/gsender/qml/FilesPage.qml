import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page
    property var controller
    property var fileModel
    signal programLoaded()

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 12
        spacing: 8

        RowLayout {
            Layout.fillWidth: true
            Label {
                text: qsTr("Programs (%1)").arg(fileModel.count)
                font.pixelSize: 18; font.bold: true
                Layout.fillWidth: true
            }
            Button {
                text: qsTr("Refresh")
                Layout.preferredHeight: 48
                font.pixelSize: 16
                onClicked: fileModel.refresh()
            }
        }

        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 2

            ListView {
                id: list
                anchors.fill: parent
                clip: true
                model: fileModel
                spacing: 2

                delegate: ItemDelegate {
                    width: ListView.view.width
                    height: 60
                    onClicked: {
                        if (page.controller.loadProgram(model.path))
                            page.programLoaded()
                    }
                    contentItem: RowLayout {
                        spacing: 10
                        Rectangle {
                            width: 46; height: 30; radius: 4
                            color: model.source === "usb" ? "#1565c0" : "#616161"
                            Label {
                                anchors.centerIn: parent
                                text: model.source === "usb" ? "USB" : "SD"
                                color: "white"; font.pixelSize: 12; font.bold: true
                            }
                        }
                        Label {
                            text: model.name
                            font.pixelSize: 18
                            elide: Text.ElideRight
                            Layout.fillWidth: true
                        }
                        Label {
                            text: (model.size / 1024).toFixed(1) + " kB"
                            font.pixelSize: 14; color: "#757575"
                        }
                    }
                }

                Label {
                    anchors.centerIn: parent
                    visible: list.count === 0
                    text: qsTr("No .nc files found.\nCopy them to /data or plug in a USB stick.")
                    horizontalAlignment: Text.AlignHCenter
                    font.pixelSize: 16; color: "#9e9e9e"
                }
            }
        }

        Label {
            Layout.fillWidth: true
            visible: page.controller.programName !== ""
            text: qsTr("Loaded: %1  (%2 lines)")
                    .arg(page.controller.programName).arg(page.controller.programLines)
            font.pixelSize: 16; color: "#2e7d32"
        }
    }
}
