import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

Item {
    id: page
    property var cam

    function num(cb) { return Number(cb.currentText) }
    function params() {
        return {
            width: num(widthCb), cutDepth: num(depthCb), tipAngle: num(angleCb),
            tipDia: 0.1, passes: num(passesCb), overlap: num(overlapCb),
            feed: num(feedCb), plungeFeed: num(plungeCb), safeZ: 2.0, travelZ: 5.0,
            spindleRpm: num(rpmCb), drillDepth: num(ddepthCb), drillFeed: num(dfeedCb)
        }
    }

    FolderDialog {
        id: folderDlg
        title: qsTr("Select Gerber folder")
        onAccepted: cam.scanFolder(selectedFolder)
    }
    FileDialog {
        id: alignDlg
        title: qsTr("Select alignment drill (.drl)")
        nameFilters: ["Excellon (*.drl)", "All files (*)"]
        onAccepted: cam.setAlignFile(selectedFile)
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 12

        // ---- Controls ----
        ScrollView {
            Layout.preferredWidth: 430
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 6

                Label { text: qsTr("Files"); font.pixelSize: 17; font.bold: true }
                RowLayout {
                    Layout.fillWidth: true
                    Button { text: qsTr("Scan folder"); Layout.fillWidth: true
                             Layout.preferredHeight: 44; font.pixelSize: 15
                             onClicked: folderDlg.open() }
                    Button { text: qsTr("Align .drl"); Layout.fillWidth: true
                             Layout.preferredHeight: 44; font.pixelSize: 15
                             onClicked: alignDlg.open() }
                }
                Label {
                    Layout.fillWidth: true; font.pixelSize: 12; color: "#555"
                    wrapMode: Text.Wrap
                    text: "F_Cu: " + (cam.frontGerber || "—")
                          + "\nB_Cu: " + (cam.backGerber || "—")
                          + "\ndrill: " + (cam.drillFile || "—")
                          + "\nalign: " + (cam.alignFile || "—")
                          + "  (" + cam.alignHoleCount + " holes)"
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#e0e0e0" }

                Label { text: qsTr("V-bit / passes"); font.pixelSize: 17; font.bold: true }
                GridLayout {
                    columns: 4; columnSpacing: 6; rowSpacing: 4; Layout.fillWidth: true

                    Label { text: qsTr("Width mm"); font.pixelSize: 13 }
                    ComboBox { id: widthCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [0.1, 0.15, 0.2, 0.3, 0.4]; currentIndex: 2 }
                    Label { text: qsTr("Depth mm"); font.pixelSize: 13 }
                    ComboBox { id: depthCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [0.05, 0.06, 0.08, 0.1, 0.15]; currentIndex: 1 }

                    Label { text: qsTr("Tip °"); font.pixelSize: 13 }
                    ComboBox { id: angleCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [10, 15, 20, 30, 45, 60]; currentIndex: 3 }
                    Label { text: qsTr("Passes"); font.pixelSize: 13 }
                    ComboBox { id: passesCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [2, 3, 4, 5]; currentIndex: 0 }

                    Label { text: qsTr("Overlap"); font.pixelSize: 13 }
                    ComboBox { id: overlapCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [0.1, 0.2, 0.3, 0.5]; currentIndex: 2 }
                    Label { text: qsTr("Feed"); font.pixelSize: 13 }
                    ComboBox { id: feedCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [60, 100, 120, 200, 300]; currentIndex: 2 }

                    Label { text: qsTr("Plunge"); font.pixelSize: 13 }
                    ComboBox { id: plungeCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [30, 60, 100]; currentIndex: 1 }
                    Label { text: qsTr("RPM"); font.pixelSize: 13 }
                    ComboBox { id: rpmCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [0, 10000, 15000, 20000]; currentIndex: 0 }

                    Label { text: qsTr("Drill Z"); font.pixelSize: 13 }
                    ComboBox { id: ddepthCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [1.6, 1.8, 2.0]; currentIndex: 1 }
                    Label { text: qsTr("Drill F"); font.pixelSize: 13 }
                    ComboBox { id: dfeedCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [30, 60, 100]; currentIndex: 1 }
                }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#e0e0e0" }

                Label { text: qsTr("Generate → /data"); font.pixelSize: 17; font.bold: true }
                GridLayout {
                    columns: 2; columnSpacing: 6; rowSpacing: 6; Layout.fillWidth: true
                    Button { text: qsTr("Top isolation"); Layout.fillWidth: true
                             Layout.preferredHeight: 48; font.pixelSize: 15; highlighted: true
                             enabled: !cam.busy && cam.frontGerber !== ""
                             onClicked: cam.generateTop(page.params()) }
                    Button { text: qsTr("Bottom isolation"); Layout.fillWidth: true
                             Layout.preferredHeight: 48; font.pixelSize: 15; highlighted: true
                             enabled: !cam.busy && cam.backGerber !== ""
                             onClicked: cam.generateBottom(page.params()) }
                    Button { text: qsTr("Drill (top)"); Layout.fillWidth: true
                             Layout.preferredHeight: 44; font.pixelSize: 15
                             enabled: !cam.busy && cam.drillFile !== ""
                             onClicked: cam.generateDrill(page.params(), false) }
                    Button { text: qsTr("Drill (bottom)"); Layout.fillWidth: true
                             Layout.preferredHeight: 44; font.pixelSize: 15
                             enabled: !cam.busy && cam.drillFile !== ""
                             onClicked: cam.generateDrill(page.params(), true) }
                }
            }
        }

        // ---- Log ----
        Frame {
            Layout.fillWidth: true
            Layout.fillHeight: true
            padding: 4
            ScrollView {
                anchors.fill: parent
                clip: true
                TextArea {
                    readOnly: true
                    wrapMode: TextArea.Wrap
                    font.family: "monospace"; font.pixelSize: 12
                    text: cam.log
                    onTextChanged: cursorPosition = length
                }
            }
        }
    }
}
