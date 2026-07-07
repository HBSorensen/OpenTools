#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "GrblController.h"
#include "NcFileModel.h"
#include "pcb/PcbCam.h"
#include "vision/CameraAligner.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setOrganizationName(QStringLiteral("OpenTools"));
    app.setOrganizationDomain(QStringLiteral("foss.dk"));
    app.setApplicationName(QStringLiteral("gsender"));

    // Core objects live in C++ so the camera aligner can be wired to the shared
    // controller and to the QML image provider; QML accesses them as context
    // properties.
    GrblController grbl;
    NcFileModel files;
    PcbCam pcb;
    auto *frames = new FrameProvider();      // engine takes ownership below
    CameraAligner aligner;
    aligner.setController(&grbl);
    aligner.setFrameProvider(frames);

    QQmlApplicationEngine engine;
    engine.addImageProvider(QStringLiteral("camera"), frames);

    QQmlContext *ctx = engine.rootContext();
    ctx->setContextProperty(QStringLiteral("grbl"), &grbl);
    ctx->setContextProperty(QStringLiteral("files"), &files);
    ctx->setContextProperty(QStringLiteral("pcb"), &pcb);
    ctx->setContextProperty(QStringLiteral("aligner"), &aligner);

    engine.load(QUrl(QStringLiteral("qrc:/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
