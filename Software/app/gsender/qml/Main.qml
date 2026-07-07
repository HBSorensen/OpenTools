import QtQuick
import QtQuick.Window
import QtQuick.Controls
import QtQuick.Layouts

// Core objects (grbl, files, pcb, aligner) are provided as context properties
// from main.cpp.
ApplicationWindow {
    id: win
    visible: true
    width: 800
    height: 480
    visibility: Window.FullScreen
    title: "OpenTools gsender"

    // Persistent digital readout across the top.
    header: Dro { controller: grbl }

    // Touch navigation across the bottom.
    footer: TabBar {
        id: tabs
        TabButton { text: qsTr("Connect"); font.pixelSize: 15 }
        TabButton { text: qsTr("Files");   font.pixelSize: 15 }
        TabButton { text: qsTr("Jog");     font.pixelSize: 15 }
        TabButton { text: qsTr("Probe");   font.pixelSize: 15 }
        TabButton { text: qsTr("PCB");     font.pixelSize: 15 }
        TabButton { text: qsTr("Align");   font.pixelSize: 15 }
        TabButton { text: qsTr("Run");     font.pixelSize: 15 }
    }

    StackLayout {
        anchors.fill: parent
        currentIndex: tabs.currentIndex

        ConnectPage { controller: grbl }
        FilesPage {
            controller: grbl
            fileModel: files
            onProgramLoaded: tabs.currentIndex = 6   // jump to Run
        }
        JogPage { controller: grbl }
        ProbePage { controller: grbl }
        PcbPage { cam: pcb }
        AlignPage { controller: grbl; cam: pcb; vision: aligner }
        RunPage { controller: grbl }
    }
}
