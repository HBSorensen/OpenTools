import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

Item {
    id: page
    property var controller

    // Chosen grid origin (work coords). Set from the live position by a button so
    // the operator never has to type on a keyboard-less touch panel.
    property real originX: 0
    property real originY: 0

    readonly property bool canProbe: controller.connected
                                     && !controller.running
                                     && !controller.probing

    // Small helper: numeric value of a preset ComboBox.
    function num(cb) { return Number(cb.currentText) }

    RowLayout {
        anchors.fill: parent
        anchors.margins: 10
        spacing: 12

        // ================= Controls =================
        ScrollView {
            Layout.preferredWidth: 430
            Layout.fillHeight: true
            clip: true
            contentWidth: availableWidth

            ColumnLayout {
                width: parent.width
                spacing: 6

                // --- Z touch-plate probe ---
                Label { text: qsTr("Z touch-plate probe"); font.pixelSize: 17; font.bold: true }
                GridLayout {
                    columns: 2
                    columnSpacing: 8; rowSpacing: 4
                    Layout.fillWidth: true

                    Label { text: qsTr("Plate (mm)"); font.pixelSize: 14 }
                    ComboBox { id: plateCb; Layout.fillWidth: true; font.pixelSize: 15
                               model: [0, 0.1, 0.5, 1.0, 1.5, 2.0]; currentIndex: 0 }

                    Label { text: qsTr("Feed (mm/min)"); font.pixelSize: 14 }
                    ComboBox { id: zFeedCb; Layout.fillWidth: true; font.pixelSize: 15
                               model: [25, 50, 100, 200]; currentIndex: 1 }
                }
                Button {
                    text: qsTr("Probe Z")
                    Layout.fillWidth: true; Layout.preferredHeight: 50
                    font.pixelSize: 17; highlighted: true
                    enabled: page.canProbe
                    onClicked: controller.probeZero(page.num(plateCb), page.num(zFeedCb),
                                                    page.num(depthCb), page.num(clearCb))
                }
                Label {
                    Layout.fillWidth: true
                    text: qsTr("Last probe Z: %1").arg(controller.lastProbeZ.toFixed(3))
                    font.pixelSize: 13; color: "#757575"
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.topMargin: 4; Layout.bottomMargin: 4
                    height: 1; color: "#e0e0e0"
                }

                // --- Grid height map ---
                Label { text: qsTr("Height map"); font.pixelSize: 17; font.bold: true }
                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        text: qsTr("Origin = current XY")
                        Layout.fillWidth: true; Layout.preferredHeight: 44
                        font.pixelSize: 14
                        enabled: controller.connected
                        onClicked: { page.originX = controller.wx; page.originY = controller.wy }
                    }
                    Label {
                        text: "(" + page.originX.toFixed(1) + ", " + page.originY.toFixed(1) + ")"
                        font.pixelSize: 14; color: "#555"
                    }
                }
                GridLayout {
                    columns: 2
                    columnSpacing: 8; rowSpacing: 4
                    Layout.fillWidth: true

                    Label { text: qsTr("Width (mm)"); font.pixelSize: 14 }
                    ComboBox { id: widthCb; Layout.fillWidth: true; font.pixelSize: 15
                               model: [20, 40, 60, 80, 100, 150, 200]; currentIndex: 0 }

                    Label { text: qsTr("Height (mm)"); font.pixelSize: 14 }
                    ComboBox { id: heightCb; Layout.fillWidth: true; font.pixelSize: 15
                               model: [20, 40, 60, 80, 100, 150, 200]; currentIndex: 0 }

                    // Distance between probe points (default 5 mm).
                    Label { text: qsTr("Spacing (mm)"); font.pixelSize: 14 }
                    ComboBox { id: spacingCb; Layout.fillWidth: true; font.pixelSize: 15
                               model: [2, 3, 5, 10]; currentIndex: 2 }

                    // Auto-level sub-segments per probe cell (default 5).
                    Label { text: qsTr("Interpolations"); font.pixelSize: 14 }
                    ComboBox { id: interpCb; Layout.fillWidth: true; font.pixelSize: 15
                               model: [1, 2, 5, 10]; currentIndex: 2
                               onActivated: controller.interpolations = page.num(interpCb) }

                    Label { text: qsTr("Probe feed"); font.pixelSize: 14 }
                    ComboBox { id: gFeedCb; Layout.fillWidth: true; font.pixelSize: 15
                               model: [25, 50, 100]; currentIndex: 1 }

                    Label { text: qsTr("Max depth"); font.pixelSize: 14 }
                    ComboBox { id: depthCb; Layout.fillWidth: true; font.pixelSize: 15
                               model: [5, 10, 20]; currentIndex: 1 }

                    Label { text: qsTr("Clearance"); font.pixelSize: 14 }
                    ComboBox { id: clearCb; Layout.fillWidth: true; font.pixelSize: 15
                               model: [2, 5, 10]; currentIndex: 1 }
                }
                RowLayout {
                    Layout.fillWidth: true
                    Button {
                        text: qsTr("Probe grid")
                        Layout.fillWidth: true; Layout.preferredHeight: 50
                        font.pixelSize: 17; highlighted: true
                        enabled: page.canProbe
                        onClicked: {
                            controller.interpolations = page.num(interpCb)
                            controller.startHeightMap(
                                page.originX, page.originY,
                                page.num(widthCb), page.num(heightCb),
                                page.num(spacingCb),
                                page.num(gFeedCb), page.num(depthCb),
                                page.num(clearCb))
                        }
                    }
                    Button {
                        text: qsTr("Clear")
                        Layout.preferredHeight: 50; Layout.preferredWidth: 90
                        font.pixelSize: 16
                        enabled: controller.haveHeightMap && !controller.probing
                        onClicked: controller.clearHeightMap()
                    }
                }
            }
        }

        // ================= Status + heatmap =================
        ColumnLayout {
            Layout.fillWidth: true
            Layout.fillHeight: true
            spacing: 8

            // Auto-level toggle.
            RowLayout {
                Layout.fillWidth: true
                Switch {
                    id: autoSw
                    text: qsTr("Auto-level")
                    font.pixelSize: 16
                    enabled: controller.haveHeightMap
                    checked: controller.autoLevel
                    onToggled: controller.setAutoLevel(checked)
                }
                Item { Layout.fillWidth: true }
                Label {
                    visible: controller.haveHeightMap
                    text: controller.mapCols + "×" + controller.mapRows
                    font.pixelSize: 14; color: "#555"
                }
            }

            // Probing progress / cancel.
            RowLayout {
                Layout.fillWidth: true
                visible: controller.probing
                spacing: 8
                ProgressBar {
                    Layout.fillWidth: true
                    from: 0; to: 1; value: controller.probeProgress
                }
                Button {
                    text: qsTr("Cancel")
                    font.pixelSize: 14
                    onClicked: controller.cancelProbe()
                }
            }
            Label {
                Layout.fillWidth: true
                visible: controller.probing
                text: controller.probeInfo
                font.pixelSize: 13; color: "#555"
            }

            // Heatmap.
            Frame {
                Layout.fillWidth: true
                Layout.fillHeight: true
                padding: 6

                Item {
                    anchors.fill: parent

                    Label {
                        anchors.centerIn: parent
                        visible: !controller.haveHeightMap
                        text: qsTr("No height map yet")
                        color: "#9e9e9e"; font.pixelSize: 15
                    }

                    Grid {
                        anchors.centerIn: parent
                        visible: controller.haveHeightMap
                        columns: controller.mapCols
                        spacing: 2

                        Repeater {
                            model: controller.haveHeightMap
                                   ? controller.mapCols * controller.mapRows : 0
                            Rectangle {
                                width: 34; height: 34; radius: 3
                                property int col: index % controller.mapCols
                                // Row 0 at the bottom so it reads like the XY plane.
                                property int row: controller.mapRows - 1
                                             - Math.floor(index / controller.mapCols)
                                color: {
                                    var _ = controller.mapMinZ + controller.mapMaxZ // dep: heightMapChanged
                                    var span = controller.mapMaxZ - controller.mapMinZ
                                    var v = controller.mapZAt(col, row)
                                    var t = span > 1e-9 ? (v - controller.mapMinZ) / span : 0.5
                                    // blue (low) -> white -> red (high)
                                    return t < 0.5
                                        ? Qt.rgba(2*t, 2*t, 1, 1)
                                        : Qt.rgba(1, 2*(1-t), 2*(1-t), 1)
                                }
                                Label {
                                    anchors.centerIn: parent
                                    text: controller.mapZAt(col, row).toFixed(2)
                                    font.pixelSize: 10
                                    color: "#000"
                                }
                            }
                        }
                    }
                }
            }

            Label {
                Layout.fillWidth: true
                visible: controller.haveHeightMap
                text: qsTr("Surface span: %1 mm  (min %2, max %3)")
                        .arg((controller.mapMaxZ - controller.mapMinZ).toFixed(3))
                        .arg(controller.mapMinZ.toFixed(3))
                        .arg(controller.mapMaxZ.toFixed(3))
                font.pixelSize: 13; color: "#555"
            }
        }
    }
}
