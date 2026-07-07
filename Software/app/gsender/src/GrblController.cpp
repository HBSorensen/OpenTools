#include "GrblController.h"

#include <QSerialPortInfo>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QUrl>
#include <algorithm>
#include <cmath>
#include <utility>

namespace {
// GRBL 1.1 / grblHAL real-time command bytes.
constexpr char RT_RESET       = 0x18; // Ctrl-X soft reset
constexpr char RT_STATUS      = '?';
constexpr char RT_HOLD        = '!';
constexpr char RT_RESUME      = '~';
constexpr char RT_JOG_CANCEL  = char(0x85);
constexpr char RT_FEED_P10    = char(0x91);
constexpr char RT_FEED_M10    = char(0x92);
constexpr char RT_FEED_P1     = char(0x93);
constexpr char RT_FEED_M1     = char(0x94);
constexpr int  CONSOLE_MAX    = 4000; // keep the console text bounded
}

GrblController::GrblController(QObject *parent) : QObject(parent)
{
    m_statusTimer.setInterval(200); // ~5 Hz status polling
    connect(&m_statusTimer, &QTimer::timeout, this, &GrblController::pollStatus);
    connect(&m_port, &QSerialPort::readyRead, this, &GrblController::onReadyRead);
    connect(&m_port, &QSerialPort::errorOccurred, this, &GrblController::onErrorOccurred);
    refreshPorts();
}

GrblController::~GrblController()
{
    close();
}

double GrblController::progress() const
{
    if (m_program.isEmpty())
        return 0.0;
    return double(m_acked) / double(m_program.size());
}

double GrblController::probeProgress() const
{
    if (m_probe == Probe::Mapping && !m_order.isEmpty())
        return double(m_ptIndex) / double(m_order.size());
    return 0.0;
}

QString GrblController::probeInfo() const
{
    switch (m_probe) {
    case Probe::Zeroing: return QStringLiteral("Probing Z…");
    case Probe::Mapping:
        return QStringLiteral("Probing point %1 / %2")
            .arg(std::min(m_ptIndex + 1, m_order.size())).arg(m_order.size());
    default: return QString();
    }
}

// --- Connection --------------------------------------------------------------

void GrblController::refreshPorts()
{
    QStringList ports;
    for (const QSerialPortInfo &info : QSerialPortInfo::availablePorts()) {
        const QString name = info.portName();
        if (name.startsWith("ttyACM") || name.startsWith("ttyUSB") || name.startsWith("ttyAMA"))
            ports << name;
    }
    std::sort(ports.begin(), ports.end());
    if (ports != m_availablePorts) {
        m_availablePorts = ports;
        emit availablePortsChanged();
    }
}

bool GrblController::open(const QString &port, int baud)
{
    close();
    m_port.setPortName(port);
    m_port.setBaudRate(baud);
    m_port.setDataBits(QSerialPort::Data8);
    m_port.setParity(QSerialPort::NoParity);
    m_port.setStopBits(QSerialPort::OneStop);
    m_port.setFlowControl(QSerialPort::NoFlowControl);

    if (!m_port.open(QIODevice::ReadWrite)) {
        appendConsole(QStringLiteral("!! Failed to open %1: %2")
                          .arg(port, m_port.errorString()));
        return false;
    }

    m_rx.clear();
    m_queue.clear();
    m_pending.clear();
    m_state = QStringLiteral("Connecting");
    appendConsole(QStringLiteral("== Opened %1 @ %2 baud").arg(port).arg(baud));
    emit connectedChanged();
    emit stateChanged();

    softReset();          // wake the controller; it replies with a welcome banner
    m_statusTimer.start();
    return true;
}

void GrblController::close()
{
    m_statusTimer.stop();
    if (m_port.isOpen())
        m_port.close();
    m_queue.clear();
    m_pending.clear();
    if (m_running) {
        m_running = false;
        emit runningChanged();
    }
    if (m_probe != Probe::Idle) {
        m_probe = Probe::Idle;
        emit probingChanged();
    }
    m_state = QStringLiteral("Unknown");
    emit stateChanged();
    emit connectedChanged();
}

// --- Outgoing --------------------------------------------------------------

void GrblController::writeRealtime(char byte)
{
    if (!m_port.isOpen())
        return;
    m_port.write(&byte, 1);
    m_port.flush();
}

void GrblController::enqueue(const QString &line)
{
    const QString trimmed = line.trimmed();
    if (trimmed.isEmpty())
        return;
    m_queue << trimmed;
    pump();
}

void GrblController::pump()
{
    while (!m_queue.isEmpty()) {
        QByteArray data = m_queue.front().toLatin1();
        data.append('\n');

        int used = 0;
        for (int n : m_pending)
            used += n;
        if (used + data.size() > RX_BUFFER)
            break; // controller RX buffer would overflow — wait for an ack

        m_port.write(data);
        m_pending.append(data.size());
        m_queue.pop_front();
    }
    m_port.flush();
}

void GrblController::sendCommand(const QString &line)
{
    if (m_running || m_probe != Probe::Idle)
        return;
    appendConsole(QStringLiteral(">> ") + line.trimmed());
    enqueue(line);
}

void GrblController::jog(const QString &axis, double distance, double feed)
{
    if (m_running || m_probe != Probe::Idle)
        return;
    enqueue(QStringLiteral("$J=G91 %1%2 F%3")
                .arg(axis)
                .arg(distance, 0, 'f', 3)
                .arg(feed, 0, 'f', 0));
}

void GrblController::jogCancel()      { writeRealtime(RT_JOG_CANCEL); }
void GrblController::home()           { sendCommand(QStringLiteral("$H")); }
void GrblController::unlock()         { sendCommand(QStringLiteral("$X")); }
void GrblController::zeroWork()       { sendCommand(QStringLiteral("G10 L20 P1 X0 Y0 Z0")); }
void GrblController::feedHold()       { writeRealtime(RT_HOLD); }
void GrblController::resume()         { writeRealtime(RT_RESUME); }

void GrblController::softReset()
{
    writeRealtime(RT_RESET);
    m_queue.clear();
    m_pending.clear();
    m_acked = 0;
    if (m_running) {
        m_running = false;
        emit runningChanged();
    }
    if (m_probe != Probe::Idle) {
        m_probe = Probe::Idle;
        m_zeroStage = 0;
        m_order.clear();
        emit probingChanged();
    }
    emit progressChanged();
    appendConsole(QStringLiteral("== soft reset"));
}

void GrblController::setFeedOverride(int percent)
{
    if (!m_port.isOpen())
        return;
    int target = std::clamp(percent, 10, 200);
    int diff = target - m_feedOv;
    while (diff >= 10) { writeRealtime(RT_FEED_P10); diff -= 10; }
    while (diff <= -10) { writeRealtime(RT_FEED_M10); diff += 10; }
    while (diff >= 1) { writeRealtime(RT_FEED_P1); diff -= 1; }
    while (diff <= -1) { writeRealtime(RT_FEED_M1); diff += 1; }
    m_feedOv = target; // optimistic; the next status report confirms it
    emit positionChanged();
}

// --- Program streaming -------------------------------------------------------

bool GrblController::loadProgram(const QString &path)
{
    QString p = path;
    if (p.startsWith("file://"))
        p = QUrl(p).toLocalFile();

    QFile f(p);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        appendConsole(QStringLiteral("!! Cannot open %1").arg(p));
        return false;
    }

    m_rawProgram.clear();
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty())
            continue;               // skip blanks so progress stays meaningful
        m_rawProgram << line;
    }
    m_programName = QFileInfo(p).fileName();
    appendConsole(QStringLiteral("== Loaded %1 (%2 lines)")
                      .arg(m_programName).arg(m_rawProgram.size()));
    rebuildProgram();
    return !m_rawProgram.isEmpty();
}

void GrblController::startProgram()
{
    if (!m_port.isOpen() || m_program.isEmpty() || m_running || m_probe != Probe::Idle)
        return;
    m_acked = 0;
    m_queue.clear();
    m_pending.clear();
    m_running = true;
    emit runningChanged();
    emit progressChanged();
    appendConsole(QStringLiteral("== Streaming %1%2")
                      .arg(m_programName,
                           m_autoLevel && m_map.valid() ? QStringLiteral(" (auto-levelled)")
                                                        : QString()));
    for (const QString &line : std::as_const(m_program))
        m_queue << line;
    pump();
}

void GrblController::stopProgram()
{
    softReset(); // flushes the controller and clears queues; also drops m_running
}

void GrblController::finishRun(const QString &reason)
{
    if (!m_running)
        return;
    m_running = false;
    emit runningChanged();
    appendConsole(QStringLiteral("== %1").arg(reason));
}

// --- Probing / auto-level ----------------------------------------------------

void GrblController::probeZero(double plateThickness, double feed,
                               double maxDepth, double clearance)
{
    if (!m_port.isOpen() || m_running || m_probe != Probe::Idle)
        return;
    m_pThick = plateThickness;
    m_pFeed = feed;
    m_pDepth = maxDepth;
    m_pClear = clearance;
    m_probe = Probe::Zeroing;
    m_zeroStage = 0;
    emit probingChanged();
    appendConsole(QStringLiteral("== Probing Z (touch plate)…"));
    enqueue(QStringLiteral("G38.2 Z-%1 F%2").arg(m_pDepth).arg(m_pFeed));
}

void GrblController::startHeightMap(double x0, double y0, double width, double height,
                                   double spacing, double feed,
                                   double maxDepth, double clearance)
{
    if (!m_port.isOpen() || m_running || m_probe != Probe::Idle)
        return;
    if (spacing < 0.5) spacing = 5.0;

    // Derive the grid dimensions from the requested probe-point spacing.
    int cols = std::max(2, int(std::lround(width / spacing)) + 1);
    int rows = std::max(2, int(std::lround(height / spacing)) + 1);

    m_pFeed = feed;
    m_pDepth = maxDepth;
    m_pClear = clearance;
    m_map.configure(x0, y0, width, height, cols, rows);

    // Snake (boustrophedon) visiting order to minimize travel.
    m_order.clear();
    for (int r = 0; r < rows; ++r) {
        if (r % 2 == 0) {
            for (int c = 0; c < cols; ++c)
                m_order.push_back({c, r, m_map.pointX(c), m_map.pointY(r)});
        } else {
            for (int c = cols - 1; c >= 0; --c)
                m_order.push_back({c, r, m_map.pointX(c), m_map.pointY(r)});
        }
    }

    m_ptIndex = 0;
    m_probe = Probe::Mapping;
    emit probingChanged();
    emit heightMapChanged();
    appendConsole(QStringLiteral("== Height map: probing %1 x %2 points…")
                      .arg(cols).arg(rows));
    probeNextPoint();
}

void GrblController::probeNextPoint()
{
    if (m_probe != Probe::Mapping)
        return;
    if (m_ptIndex >= m_order.size()) {
        finishHeightMap();
        return;
    }
    const ProbePt &p = m_order[m_ptIndex];
    enqueue(QStringLiteral("G0 Z%1").arg(m_pClear));                 // retract
    enqueue(QStringLiteral("G0 X%1 Y%2").arg(p.x, 0, 'f', 3).arg(p.y, 0, 'f', 3));
    enqueue(QStringLiteral("G38.2 Z-%1 F%2").arg(m_pDepth).arg(m_pFeed)); // probe
}

void GrblController::finishHeightMap()
{
    enqueue(QStringLiteral("G0 Z%1").arg(m_pClear));
    enqueue(QStringLiteral("G0 X%1 Y%2")
                .arg(m_map.x0(), 0, 'f', 3).arg(m_map.y0(), 0, 'f', 3));
    m_map.finalize();
    m_probe = Probe::Idle;
    emit probingChanged();
    emit heightMapChanged();
    appendConsole(QStringLiteral("== Height map complete: span %1 mm (min %2, max %3)")
                      .arg(m_map.maxZ() - m_map.minZ(), 0, 'f', 3)
                      .arg(m_map.minZ(), 0, 'f', 3)
                      .arg(m_map.maxZ(), 0, 'f', 3));
    if (m_autoLevel)
        rebuildProgram();
}

void GrblController::onProbeIdle()
{
    if (m_probe == Probe::Zeroing) {
        if (m_zeroStage >= 1) {          // zero applied + retracted
            m_probe = Probe::Idle;
            emit probingChanged();
            appendConsole(QStringLiteral("== Z set (plate %1 mm)").arg(m_pThick));
        }
    } else if (m_probe == Probe::Mapping) {
        ++m_ptIndex;
        emit probingChanged();
        probeNextPoint();                // advances or finishes
    }
}

void GrblController::abortProbe(const QString &reason)
{
    if (m_probe == Probe::Idle)
        return;
    m_probe = Probe::Idle;
    m_zeroStage = 0;
    m_order.clear();
    m_queue.clear();
    emit probingChanged();
    appendConsole(QStringLiteral("!! Probe aborted: %1").arg(reason));
}

void GrblController::cancelProbe()
{
    if (m_probe == Probe::Idle)
        return;
    abortProbe(QStringLiteral("cancelled"));
    softReset();
}

void GrblController::clearHeightMap()
{
    m_map.clear();
    emit heightMapChanged();
    appendConsole(QStringLiteral("== Height map cleared"));
    if (m_autoLevel)
        rebuildProgram();
}

void GrblController::setAutoLevel(bool on)
{
    if (m_autoLevel == on)
        return;
    m_autoLevel = on;
    emit autoLevelChanged();
    rebuildProgram();
}

void GrblController::setInterpolations(int n)
{
    n = std::max(1, n);
    if (m_interpolations == n)
        return;
    m_interpolations = n;
    emit interpolationsChanged();
    rebuildProgram();  // re-apply with the new segment resolution
}

void GrblController::rebuildProgram()
{
    if (m_autoLevel && m_map.valid() && !m_rawProgram.isEmpty()) {
        QString warn;
        // Interpolation segment length = probe spacing / number of interpolations
        // (e.g. 5 mm grid / 5 = 1 mm Z-correction steps).
        double spacing = std::min(m_map.dx(), m_map.dy());
        double segLen = spacing / std::max(1, m_interpolations);
        m_program = applyHeightMap(m_rawProgram, m_map, segLen, warn);
        if (!warn.isEmpty())
            appendConsole(QStringLiteral("!! ") + warn);
        appendConsole(QStringLiteral("== Auto-level applied (%1 -> %2 lines)")
                          .arg(m_rawProgram.size()).arg(m_program.size()));
    } else {
        m_program = m_rawProgram;
    }
    m_acked = 0;
    emit programChanged();
    emit progressChanged();
}

// --- Incoming ----------------------------------------------------------------

void GrblController::onReadyRead()
{
    m_rx += m_port.readAll();
    int idx;
    while ((idx = m_rx.indexOf('\n')) >= 0) {
        const QString line = QString::fromLatin1(m_rx.left(idx)).trimmed();
        m_rx.remove(0, idx + 1);
        if (!line.isEmpty())
            handleLine(line);
    }
}

void GrblController::onErrorOccurred(QSerialPort::SerialPortError error)
{
    if (error == QSerialPort::NoError)
        return;
    if (error == QSerialPort::ResourceError) {
        appendConsole(QStringLiteral("!! Serial resource lost — disconnected"));
        close();
    }
}

void GrblController::handleLine(const QString &line)
{
    if (line == QLatin1String("ok") || line.startsWith(QLatin1String("error"))) {
        if (!m_pending.isEmpty())
            m_pending.removeFirst();
        if (line.startsWith(QLatin1String("error")))
            appendConsole(QStringLiteral("<< ") + line);
        if (m_running) {
            ++m_acked;
            emit progressChanged();
            if (m_acked >= m_program.size())
                finishRun(QStringLiteral("Program complete"));
        }
        pump();
        // Probe sequences advance once the controller is idle (all lines acked).
        if (m_probe != Probe::Idle && m_queue.isEmpty() && m_pending.isEmpty())
            onProbeIdle();
        return;
    }

    if (line.startsWith('<')) {          // real-time status report
        parseStatus(line);
        return;
    }

    if (line.startsWith(QLatin1String("[PRB:"))) {
        parseProbe(line);
        return;
    }

    if (line.startsWith(QLatin1String("ALARM"))) {
        m_state = QStringLiteral("Alarm");
        emit stateChanged();
        appendConsole(QStringLiteral("<< ") + line);
        m_queue.clear();
        m_pending.clear();
        if (m_probe != Probe::Idle)
            abortProbe(line);
        finishRun(QStringLiteral("Halted (") + line + QChar(')'));
        return;
    }

    // Welcome banner, [MSG:...], $-settings echoes, etc.
    appendConsole(QStringLiteral("<< ") + line);
}

void GrblController::parseProbe(const QString &report)
{
    // Format: [PRB:x,y,z:result]
    appendConsole(QStringLiteral("<< ") + report);
    QString inner = report;
    inner.remove('[').remove(']');
    if (inner.startsWith(QLatin1String("PRB:")))
        inner = inner.mid(4);
    const QStringList halves = inner.split(':');
    const QStringList nums = halves.value(0).split(',');
    const bool ok = halves.value(1).trimmed() == QLatin1String("1");

    double pz = nums.value(2).toDouble();
    const double workZ = pz - m_wco[2];   // machine -> work Z

    if (!ok) {
        appendConsole(QStringLiteral("!! Probe did not make contact"));
        return; // an ALARM typically follows and aborts the sequence
    }

    m_lastProbeZ = workZ;
    emit lastProbeZChanged();

    if (m_probe == Probe::Zeroing) {
        enqueue(QStringLiteral("G10 L20 P1 Z%1").arg(m_pThick)); // set work Z
        enqueue(QStringLiteral("G0 Z%1").arg(m_pClear));         // retract
        m_zeroStage = 1;
    } else if (m_probe == Probe::Mapping && m_ptIndex < m_order.size()) {
        const ProbePt &p = m_order[m_ptIndex];
        m_map.setZ(p.col, p.row, workZ);
    }
}

void GrblController::parseStatus(const QString &report)
{
    // Format: <State|MPos:x,y,z|FS:f,s|WCO:x,y,z|Ov:f,r,s|...>
    QString body = report;
    body.remove('<').remove('>');
    const QStringList fields = body.split('|', Qt::SkipEmptyParts);
    if (fields.isEmpty())
        return;

    bool haveMPos = false, haveWPos = false;
    double pos[3] = {0, 0, 0};

    QString newState = fields.first().section(':', 0, 0);

    for (int i = 1; i < fields.size(); ++i) {
        const QString key = fields[i].section(':', 0, 0);
        const QString val = fields[i].section(':', 1);
        const QStringList nums = val.split(',');

        auto toTriple = [&](double out[3]) {
            for (int k = 0; k < 3 && k < nums.size(); ++k)
                out[k] = nums[k].toDouble();
        };

        if (key == QLatin1String("MPos")) { toTriple(pos); haveMPos = true; }
        else if (key == QLatin1String("WPos")) { toTriple(pos); haveWPos = true; }
        else if (key == QLatin1String("WCO")) { toTriple(m_wco); }
        else if (key == QLatin1String("FS")) { if (!nums.isEmpty()) m_feed = nums[0].toDouble(); }
        else if (key == QLatin1String("F")) { if (!nums.isEmpty()) m_feed = nums[0].toDouble(); }
        else if (key == QLatin1String("Ov")) { if (!nums.isEmpty()) m_feedOv = nums[0].toInt(); }
    }

    if (haveMPos) {
        for (int k = 0; k < 3; ++k) { m_mpos[k] = pos[k]; m_wpos[k] = pos[k] - m_wco[k]; }
    } else if (haveWPos) {
        for (int k = 0; k < 3; ++k) { m_wpos[k] = pos[k]; m_mpos[k] = pos[k] + m_wco[k]; }
    }

    if (newState != m_state) {
        m_state = newState;
        emit stateChanged();
    }
    emit positionChanged();
}

void GrblController::pollStatus()
{
    if (m_port.isOpen())
        writeRealtime(RT_STATUS);
}

void GrblController::appendConsole(const QString &text)
{
    m_console += text;
    m_console += QChar('\n');
    if (m_console.size() > CONSOLE_MAX)
        m_console = m_console.right(CONSOLE_MAX);
    emit consoleChanged();
}
