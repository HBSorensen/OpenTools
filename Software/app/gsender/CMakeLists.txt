cmake_minimum_required(VERSION 3.16)
project(gsender VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick SerialPort)
find_package(Threads REQUIRED)

# OpenCV is optional: when present, camera alignment is compiled in. Without it
# the app still builds (e.g. host-side CAM testing) and reports vision unavailable.
# (No COMPONENTS filter — that would fail the whole find if one module is absent.)
find_package(OpenCV QUIET)

qt_add_executable(gsender
    src/main.cpp
    src/GrblController.cpp
    src/NcFileModel.cpp
    src/HeightMap.cpp
    src/pcb/Geometry.cpp
    src/pcb/GerberParser.cpp
    src/pcb/ExcellonParser.cpp
    src/pcb/CamProcessor.cpp
    src/pcb/PcbCam.cpp
    src/vision/CameraAligner.cpp
    # vendored Clipper2 (polygon union + offsetting)
    third_party/clipper2/src/clipper.engine.cpp
    third_party/clipper2/src/clipper.offset.cpp
    third_party/clipper2/src/clipper.rectclip.cpp
    resources.qrc
)

target_include_directories(gsender PRIVATE
    src
    third_party/clipper2/include
)

target_link_libraries(gsender PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
    Qt6::SerialPort
    Threads::Threads
)

if(OpenCV_FOUND)
    message(STATUS "gsender: OpenCV found (${OpenCV_VERSION}) — camera alignment enabled")
    target_compile_definitions(gsender PRIVATE GSENDER_HAVE_OPENCV)
    target_include_directories(gsender PRIVATE ${OpenCV_INCLUDE_DIRS})
    target_link_libraries(gsender PRIVATE ${OpenCV_LIBS})
else()
    message(WARNING "gsender: OpenCV not found — camera alignment will be disabled")
endif()

include(GNUInstallDirs)
install(TARGETS gsender RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
