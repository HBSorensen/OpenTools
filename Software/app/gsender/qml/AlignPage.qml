import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page
    property var controller
    property var cam
    property var vision

    function num(cb) { return Number(cb.currentText) }
    function expectedList() {
        var a = []
        for (var i = 0; i < cam.alignHoleCount; i++)
            a.push([cam.alignExpectedX(i), cam.alignExpectedY(i)])
        return a
    }

    // When the aligner solves the transform, apply it to the bottom program.
    Connections {
        target: vision
        function onSolved(c, s, tx, ty) { cam.applyAlignment(c, s, tx, ty) }
    }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 12

        // ---- Camera preview ----
        Rectangle {
            Layout.preferredWidth: 360
            Layout.fillHeight: true
            color: "#101010"
            border.color: "#444"

            Image {
                anchors.fill: parent
                anchors.margins: 4
                fillMode: Image.PreserveAspectFit
                cache: false
                source: vision.cameraOpen
                        ? "image://camera/frame?" + vision.frameCounter
                        : ""
            }
            Label {
                anchors.centerIn: parent
                visible: !vision.cameraOpen
                text: vision.available ? qsTr("Camera closed")
                                        : qsTr("Vision unavailable\n(built without OpenCV)")
                color: "#aaa"; horizontalAlignment: Text.AlignHCenter
            }
            // Live correction indicator.
            Label {
                anchors.left: parent.left; anchors.top: parent.top; anchors.margins: 6
                visible: vision.cameraOpen && vision.calibrated && vision.undistort
                text: qsTr("fisheye ✓"); color: "#4caf50"; font.pixelSize: 12
            }
        }

        // ---- Controls (scrollable on the 480px-tall screen) ----
        ScrollView {
            Layout.fillWidth: true
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 6

                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        text: vision.cameraOpen ? qsTr("Close camera") : qsTr("Open camera")
                        Layout.fillWidth: true; Layout.preferredHeight: 44; font.pixelSize: 15
                        enabled: vision.available
                        onClicked: vision.cameraOpen ? vision.closeCamera() : vision.openCamera(0)
                    }
                    Switch {
                        text: qsTr("Undistort"); font.pixelSize: 14
                        enabled: vision.calibrated
                        checked: vision.undistort
                        onToggled: vision.undistort = checked
                    }
                }

                // ---- Fisheye calibration ----
                Rectangle { Layout.fillWidth: true; height: 1; color: "#e0e0e0" }
                Label { text: qsTr("Fisheye calibration"); font.pixelSize: 16; font.bold: true }
                GridLayout {
                    columns: 4; columnSpacing: 6; rowSpacing: 4; Layout.fillWidth: true
                    Label { text: qsTr("Cols"); font.pixelSize: 13 }
                    ComboBox { id: colsCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [6, 7, 8, 9, 10]; currentIndex: 3
                               onActivated: vision.boardCols = page.num(colsCb) }
                    Label { text: qsTr("Rows"); font.pixelSize: 13 }
                    ComboBox { id: rowsCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [5, 6, 7, 8]; currentIndex: 1
                               onActivated: vision.boardRows = page.num(rowsCb) }
                    Label { text: qsTr("Sq mm"); font.pixelSize: 13 }
                    ComboBox { id: sqCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [5, 10, 15, 20, 25]; currentIndex: 1
                               onActivated: vision.squareSize = page.num(sqCb) }
                    Item { Layout.fillWidth: true }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        text: qsTr("Capture board")
                        Layout.fillWidth: true; Layout.preferredHeight: 46; font.pixelSize: 15
                        enabled: vision.cameraOpen
                        onClicked: vision.captureCalibrationView()
                    }
                    Button {
                        text: qsTr("Calibrate")
                        Layout.fillWidth: true; Layout.preferredHeight: 46; font.pixelSize: 15
                        highlighted: true
                        enabled: vision.calibViews >= 6
                        onClicked: vision.calibrate()
                    }
                    Button {
                        text: qsTr("Clear")
                        Layout.preferredWidth: 70; Layout.preferredHeight: 46; font.pixelSize: 14
                        onClicked: vision.clearCalibration()
                    }
                }
                Label {
                    Layout.fillWidth: true; font.pixelSize: 12; color: "#555"
                    text: vision.calibrated
                          ? qsTr("Calibrated ✓  (RMS %1 px)").arg(vision.calibError.toFixed(3))
                          : qsTr("Views captured: %1 / 6+").arg(vision.calibViews)
                }

                // ---- Hole detection / mapping ----
                Rectangle { Layout.fillWidth: true; height: 1; color: "#e0e0e0" }
                Label { text: qsTr("Detection / mapping"); font.pixelSize: 16; font.bold: true }
                GridLayout {
                    columns: 4; columnSpacing: 6; rowSpacing: 4; Layout.fillWidth: true
                    Label { text: qsTr("mm/px"); font.pixelSize: 13 }
                    ComboBox { id: mmpxCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [0.02, 0.03, 0.05, 0.08, 0.1]; currentIndex: 2
                               onActivated: vision.mmPerPixel = page.num(mmpxCb) }
                    Label { text: qsTr("min r"); font.pixelSize: 13 }
                    ComboBox { id: minrCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [3, 5, 8, 12]; currentIndex: 1
                               onActivated: vision.minRadius = page.num(minrCb) }
                    Label { text: qsTr("max r"); font.pixelSize: 13 }
                    ComboBox { id: maxrCb; Layout.fillWidth: true; font.pixelSize: 14
                               model: [40, 60, 80, 120]; currentIndex: 1
                               onActivated: vision.maxRadius = page.num(maxrCb) }
                    Label { text: qsTr("flip"); font.pixelSize: 13 }
                    RowLayout {
                        Layout.fillWidth: true; spacing: 4
                        Button { text: "X"; checkable: true; checked: vision.flipX < 0
                                 Layout.fillWidth: true; font.pixelSize: 13
                                 onClicked: vision.flipX = checked ? -1 : 1 }
                        Button { text: "Y"; checkable: true; checked: vision.flipY < 0
                                 Layout.fillWidth: true; font.pixelSize: 13
                                 onClicked: vision.flipY = checked ? -1 : 1 }
                    }
                }

                // ---- Run alignment ----
                Button {
                    text: qsTr("Start alignment (%1 holes)").arg(cam.alignHoleCount)
                    Layout.fillWidth: true; Layout.preferredHeight: 52
                    font.pixelSize: 16; highlighted: true
                    enabled: vision.cameraOpen && controller.connected
                             && cam.alignHoleCount >= 3 && !vision.running
                    onClicked: vision.beginAlignment(page.expectedList())
                }
                Button {
                    text: qsTr("Cancel")
                    Layout.fillWidth: true; Layout.preferredHeight: 38; font.pixelSize: 14
                    visible: vision.running
                    onClicked: vision.cancel()
                }

                Frame {
                    Layout.fillWidth: true
                    Layout.preferredHeight: 70
                    padding: 6
                    Label {
                        anchors.fill: parent
                        text: vision.status
                        wrapMode: Text.Wrap; font.pixelSize: 13
                        verticalAlignment: Text.AlignTop
                    }
                }
            }
        }
    }
}
