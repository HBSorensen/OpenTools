#include "PcbCam.h"

#include "GerberParser.h"
#include "ExcellonParser.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QUrl>
#include <QProcessEnvironment>

PcbCam::PcbCam(QObject *parent) : QObject(parent) {}

QString PcbCam::localPath(const QString &pathOrUrl)
{
    if (pathOrUrl.startsWith("file://"))
        return QUrl(pathOrUrl).toLocalFile();
    return pathOrUrl;
}

QString PcbCam::dataDir() const
{
    return QProcessEnvironment::systemEnvironment().value("GSENDER_DATA", "/data");
}

void PcbCam::appendLog(const QString &line)
{
    m_log += line + "\n";
    emit logChanged();
}

void PcbCam::setBusy(bool b)
{
    if (m_busy == b) return;
    m_busy = b;
    emit busyChanged();
}

// --- File selection ----------------------------------------------------------

void PcbCam::scanFolder(const QString &dir)
{
    QDir d(localPath(dir));
    if (!d.exists()) { appendLog("!! Folder not found: " + d.absolutePath()); return; }

    const QStringList files = d.entryList(QDir::Files);
    for (const QString &f : files) {
        const QString p = d.absoluteFilePath(f);
        if (f.contains("-F_Cu.", Qt::CaseInsensitive))      m_front = p;
        else if (f.contains("-B_Cu.", Qt::CaseInsensitive)) m_back = p;
        else if (f.endsWith(".drl", Qt::CaseInsensitive)
                 && !f.contains("align", Qt::CaseInsensitive)) m_drill = p;
        if (f.contains("align", Qt::CaseInsensitive) && f.endsWith(".drl", Qt::CaseInsensitive))
            m_align = p;
    }
    appendLog(QString("Scanned %1: F_Cu=%2 B_Cu=%3 drill=%4 align=%5")
                  .arg(d.dirName(),
                       QFileInfo(m_front).fileName(), QFileInfo(m_back).fileName(),
                       QFileInfo(m_drill).fileName(), QFileInfo(m_align).fileName()));
    recomputeAxis();
    if (!m_align.isEmpty()) setAlignFile(m_align);
    emit filesChanged();
}

void PcbCam::setFrontGerber(const QString &path) { m_front = localPath(path); recomputeAxis(); emit filesChanged(); }
void PcbCam::setBackGerber(const QString &path)  { m_back  = localPath(path); recomputeAxis(); emit filesChanged(); }
void PcbCam::setDrillFile(const QString &path)   { m_drill = localPath(path); emit filesChanged(); }

void PcbCam::setAlignFile(const QString &path)
{
    m_align = localPath(path);
    m_alignHoles.clear();
    QFile f(m_align);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ExcellonParser ex;
        if (ex.parse(QString::fromUtf8(f.readAll()))) {
            m_alignHoles = ex.holes();
            appendLog(QString("Alignment file: %1 holes").arg(m_alignHoles.size()));
            if (m_alignHoles.size() < 3)
                appendLog("!! Need at least 3 alignment holes for registration.");
        } else {
            appendLog("!! Alignment parse failed: " + ex.error());
        }
    }
    emit filesChanged();
}

void PcbCam::recomputeAxis()
{
    // Mirror axis = board-center X, from the front copper if available, else back.
    const QString src = !m_front.isEmpty() ? m_front : m_back;
    if (src.isEmpty()) return;
    QFile f(src);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) return;
    GerberParser g;
    if (g.parse(QString::fromUtf8(f.readAll()))) {
        m_axisX = g.bounds().cx();
        m_haveAxis = true;
    }
}

// --- Generation --------------------------------------------------------------

CamProcessor::ToolParams PcbCam::paramsFrom(const QVariantMap &m)
{
    CamProcessor::ToolParams p;
    auto d = [&](const char *k, double def) { return m.value(k, def).toDouble(); };
    p.width       = d("width", 0.2);
    p.cutDepth    = d("cutDepth", 0.06);
    p.tipAngle    = d("tipAngle", 30.0);
    p.tipDia      = d("tipDia", 0.1);
    p.passes      = std::max(2, m.value("passes", 2).toInt());
    p.overlap     = d("overlap", 0.3);
    p.feed        = d("feed", 120.0);
    p.plungeFeed  = d("plungeFeed", 60.0);
    p.safeZ       = d("safeZ", 2.0);
    p.travelZ     = d("travelZ", 5.0);
    p.spindleRpm  = d("spindleRpm", 0.0);
    p.drillDepth  = d("drillDepth", 1.8);
    p.drillFeed   = d("drillFeed", 60.0);
    return p;
}

QString PcbCam::writeNc(const QString &name, const QStringList &lines)
{
    if (lines.isEmpty()) return {};
    QDir().mkpath(dataDir());
    const QString path = QDir(dataDir()).absoluteFilePath(name);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        appendLog("!! Cannot write " + path);
        return {};
    }
    QTextStream out(&f);
    for (const QString &l : lines) out << l << "\n";
    appendLog(QString("Wrote %1 (%2 lines)").arg(name).arg(lines.size()));
    emit generated(path);
    return path;
}

QString PcbCam::generateTop(const QVariantMap &params)
{
    if (m_front.isEmpty()) { appendLog("!! No F_Cu gerber selected"); return {}; }
    setBusy(true);
    QString result;
    QFile f(m_front);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        GerberParser g;
        if (g.parse(QString::fromUtf8(f.readAll()))) {
            for (const QString &w : g.warnings()) appendLog("~ " + w);
            m_axisX = g.bounds().cx(); m_haveAxis = true;
            auto lines = CamProcessor::isolation(g.copper(), paramsFrom(params), false, m_axisX);
            result = writeNc("pcb_top.nc", lines);
        } else appendLog("!! Front parse failed: " + g.error());
    }
    setBusy(false);
    return result;
}

QString PcbCam::generateBottom(const QVariantMap &params)
{
    if (m_back.isEmpty()) { appendLog("!! No B_Cu gerber selected"); return {}; }
    setBusy(true);
    QString result;
    QFile f(m_back);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        GerberParser g;
        if (g.parse(QString::fromUtf8(f.readAll()))) {
            for (const QString &w : g.warnings()) appendLog("~ " + w);
            if (!m_haveAxis) { m_axisX = g.bounds().cx(); m_haveAxis = true; }
            m_bottomGcode = CamProcessor::isolation(g.copper(), paramsFrom(params), true, m_axisX);
            m_bottomBase = "pcb_bottom";
            result = writeNc("pcb_bottom.nc", m_bottomGcode);
        } else appendLog("!! Back parse failed: " + g.error());
    }
    setBusy(false);
    return result;
}

QString PcbCam::generateDrill(const QVariantMap &params, bool bottom)
{
    if (m_drill.isEmpty()) { appendLog("!! No drill file selected"); return {}; }
    setBusy(true);
    QString result;
    QFile f(m_drill);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        ExcellonParser ex;
        if (ex.parse(QString::fromUtf8(f.readAll()))) {
            for (const QString &w : ex.warnings()) appendLog("~ " + w);
            auto lines = CamProcessor::drill(ex.holes(), paramsFrom(params),
                                             bottom, m_axisX);
            result = writeNc(bottom ? "pcb_drill_bottom.nc" : "pcb_drill.nc", lines);
        } else appendLog("!! Drill parse failed: " + ex.error());
    }
    setBusy(false);
    return result;
}

// --- Alignment ---------------------------------------------------------------

double PcbCam::alignExpectedX(int i) const
{
    if (i < 0 || i >= m_alignHoles.size()) return 0.0;
    // Bottom side is mirrored about the board-center X.
    return 2.0 * m_axisX - m_alignHoles[i].x;
}

double PcbCam::alignExpectedY(int i) const
{
    if (i < 0 || i >= m_alignHoles.size()) return 0.0;
    return m_alignHoles[i].y;
}

QString PcbCam::applyAlignment(double c, double s, double tx, double ty)
{
    if (m_bottomGcode.isEmpty()) { appendLog("!! No bottom program to align"); return {}; }
    geo::Rigid t; t.c = c; t.s = s; t.tx = tx; t.ty = ty;
    appendLog(QString("Applying alignment: rot %1°, offset (%2, %3) mm")
                  .arg(t.rotationDeg(), 0, 'f', 3).arg(tx, 0, 'f', 3).arg(ty, 0, 'f', 3));
    QStringList aligned = CamProcessor::applyTransform(m_bottomGcode, t);
    return writeNc(m_bottomBase + "_aligned.nc", aligned);
}
