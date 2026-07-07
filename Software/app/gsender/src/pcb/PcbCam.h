#pragma once

// QML-facing orchestrator for the PCB CAM workflow: file selection (KiCad naming),
// isolation/drill G-code generation into the data partition, and applying a
// camera-derived rigid transform to the bottom-side program.

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

#include "CamProcessor.h"

class PcbCam : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString frontGerber READ frontGerber NOTIFY filesChanged)
    Q_PROPERTY(QString backGerber READ backGerber NOTIFY filesChanged)
    Q_PROPERTY(QString drillFile READ drillFile NOTIFY filesChanged)
    Q_PROPERTY(QString alignFile READ alignFile NOTIFY filesChanged)
    Q_PROPERTY(double mirrorAxisX READ mirrorAxisX NOTIFY filesChanged)
    Q_PROPERTY(int alignHoleCount READ alignHoleCount NOTIFY filesChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(QString log READ log NOTIFY logChanged)

public:
    explicit PcbCam(QObject *parent = nullptr);

    QString frontGerber() const { return m_front; }
    QString backGerber() const { return m_back; }
    QString drillFile() const { return m_drill; }
    QString alignFile() const { return m_align; }
    double mirrorAxisX() const { return m_axisX; }
    int alignHoleCount() const { return m_alignHoles.size(); }
    bool busy() const { return m_busy; }
    QString log() const { return m_log; }

    // File selection.
    Q_INVOKABLE void scanFolder(const QString &dir);   // KiCad auto-detect F_Cu/B_Cu/.drl
    Q_INVOKABLE void setFrontGerber(const QString &path);
    Q_INVOKABLE void setBackGerber(const QString &path);
    Q_INVOKABLE void setDrillFile(const QString &path);
    Q_INVOKABLE void setAlignFile(const QString &path);

    // Generation. Params is a JS object (width, cutDepth, tipAngle, tipDia, passes,
    // overlap, feed, plungeFeed, safeZ, travelZ, spindleRpm, drillDepth, drillFeed).
    // Returns the written .nc path (empty on failure).
    Q_INVOKABLE QString generateTop(const QVariantMap &params);
    Q_INVOKABLE QString generateBottom(const QVariantMap &params);
    Q_INVOKABLE QString generateDrill(const QVariantMap &params, bool bottom);

    // Alignment: expected (mirrored) bottom-side positions of the alignment holes,
    // and applying the solved rigid transform to the last-generated bottom program.
    Q_INVOKABLE double alignExpectedX(int i) const;
    Q_INVOKABLE double alignExpectedY(int i) const;
    Q_INVOKABLE QString applyAlignment(double c, double s, double tx, double ty);

signals:
    void filesChanged();
    void busyChanged();
    void logChanged();
    void generated(const QString &path);

private:
    static QString localPath(const QString &pathOrUrl);
    static CamProcessor::ToolParams paramsFrom(const QVariantMap &m);
    QString dataDir() const;
    QString writeNc(const QString &name, const QStringList &lines);
    void appendLog(const QString &line);
    void setBusy(bool b);
    void recomputeAxis();

    QString m_front, m_back, m_drill, m_align;
    double m_axisX = 0.0;
    bool m_haveAxis = false;
    QVector<ExcellonParser::Hole> m_alignHoles;
    QStringList m_bottomGcode;      // last generated bottom program (pre-alignment)
    QString m_bottomBase;           // basename for the aligned output
    bool m_busy = false;
    QString m_log;
};
