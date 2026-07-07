#pragma once

#include <QObject>
#include <QSerialPort>
#include <QStringList>
#include <QTimer>
#include <QList>
#include <QVector>

#include "HeightMap.h"

// GrblController owns the serial link to a grblHAL controller and implements the
// standard GRBL "character-counting" streaming protocol together with real-time
// control (status polling, feed hold/resume, reset, overrides, jogging), plus
// automatic probing (Z touch-plate + grid height map) and auto-level correction
// of the loaded program.
//
// It exposes everything the QML UI needs as properties/signals and is driven by
// Q_INVOKABLE methods.
class GrblController : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString portName READ portName NOTIFY connectedChanged)
    Q_PROPERTY(QStringList availablePorts READ availablePorts NOTIFY availablePortsChanged)

    Q_PROPERTY(QString state READ state NOTIFY stateChanged)   // Idle/Run/Hold/Alarm/...
    Q_PROPERTY(double mx READ mx NOTIFY positionChanged)
    Q_PROPERTY(double my READ my NOTIFY positionChanged)
    Q_PROPERTY(double mz READ mz NOTIFY positionChanged)
    Q_PROPERTY(double wx READ wx NOTIFY positionChanged)
    Q_PROPERTY(double wy READ wy NOTIFY positionChanged)
    Q_PROPERTY(double wz READ wz NOTIFY positionChanged)
    Q_PROPERTY(double feed READ feed NOTIFY positionChanged)
    Q_PROPERTY(int feedOverride READ feedOverride NOTIFY positionChanged)

    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(int programLines READ programLines NOTIFY programChanged)
    Q_PROPERTY(QString programName READ programName NOTIFY programChanged)
    Q_PROPERTY(double progress READ progress NOTIFY progressChanged)   // 0..1
    Q_PROPERTY(QString console READ console NOTIFY consoleChanged)

    // --- Probing / auto-level ---
    Q_PROPERTY(bool probing READ probing NOTIFY probingChanged)
    Q_PROPERTY(double probeProgress READ probeProgress NOTIFY probingChanged) // 0..1
    Q_PROPERTY(QString probeInfo READ probeInfo NOTIFY probingChanged)
    Q_PROPERTY(double lastProbeZ READ lastProbeZ NOTIFY lastProbeZChanged)
    Q_PROPERTY(bool haveHeightMap READ haveHeightMap NOTIFY heightMapChanged)
    Q_PROPERTY(int mapCols READ mapCols NOTIFY heightMapChanged)
    Q_PROPERTY(int mapRows READ mapRows NOTIFY heightMapChanged)
    Q_PROPERTY(double mapMinZ READ mapMinZ NOTIFY heightMapChanged)
    Q_PROPERTY(double mapMaxZ READ mapMaxZ NOTIFY heightMapChanged)
    Q_PROPERTY(bool autoLevel READ autoLevel WRITE setAutoLevel NOTIFY autoLevelChanged)
    Q_PROPERTY(int interpolations READ interpolations WRITE setInterpolations NOTIFY interpolationsChanged)

public:
    explicit GrblController(QObject *parent = nullptr);
    ~GrblController() override;

    bool connected() const { return m_port.isOpen(); }
    QString portName() const { return m_port.portName(); }
    QStringList availablePorts() const { return m_availablePorts; }

    QString state() const { return m_state; }
    double mx() const { return m_mpos[0]; }
    double my() const { return m_mpos[1]; }
    double mz() const { return m_mpos[2]; }
    double wx() const { return m_wpos[0]; }
    double wy() const { return m_wpos[1]; }
    double wz() const { return m_wpos[2]; }
    double feed() const { return m_feed; }
    int feedOverride() const { return m_feedOv; }

    bool running() const { return m_running; }
    int programLines() const { return m_program.size(); }
    QString programName() const { return m_programName; }
    double progress() const;
    QString console() const { return m_console; }

    bool probing() const { return m_probe != Probe::Idle; }
    double probeProgress() const;
    QString probeInfo() const;
    double lastProbeZ() const { return m_lastProbeZ; }
    bool haveHeightMap() const { return m_map.valid(); }
    int mapCols() const { return m_map.cols(); }
    int mapRows() const { return m_map.rows(); }
    double mapMinZ() const { return m_map.minZ(); }
    double mapMaxZ() const { return m_map.maxZ(); }
    bool autoLevel() const { return m_autoLevel; }
    void setAutoLevel(bool on);
    int interpolations() const { return m_interpolations; }
    void setInterpolations(int n);

    // --- Connection ---
    Q_INVOKABLE void refreshPorts();
    Q_INVOKABLE bool open(const QString &port, int baud = 115200);
    Q_INVOKABLE void close();

    // --- Manual commands (blocked while running or probing) ---
    Q_INVOKABLE void sendCommand(const QString &line);
    Q_INVOKABLE void jog(const QString &axis, double distance, double feed);
    Q_INVOKABLE void jogCancel();
    Q_INVOKABLE void home();
    Q_INVOKABLE void unlock();
    Q_INVOKABLE void zeroWork();

    // --- Real-time controls (safe any time) ---
    Q_INVOKABLE void softReset();
    Q_INVOKABLE void feedHold();
    Q_INVOKABLE void resume();
    Q_INVOKABLE void setFeedOverride(int percent);

    // --- Program streaming ---
    Q_INVOKABLE bool loadProgram(const QString &path);
    Q_INVOKABLE void startProgram();
    Q_INVOKABLE void stopProgram();

    // --- Probing / auto-level ---
    // Probe straight down to a touch plate and set work Z to plateThickness.
    Q_INVOKABLE void probeZero(double plateThickness, double feed,
                               double maxDepth, double clearance);
    // Probe a grid over the rectangle at (x0,y0) sized width x height, with probe
    // points `spacing` mm apart (grid dimensions are derived from the area).
    Q_INVOKABLE void startHeightMap(double x0, double y0, double width, double height,
                                    double spacing, double feed,
                                    double maxDepth, double clearance);
    Q_INVOKABLE void cancelProbe();
    Q_INVOKABLE void clearHeightMap();
    Q_INVOKABLE double mapZAt(int col, int row) const { return m_map.at(col, row); }

signals:
    void connectedChanged();
    void availablePortsChanged();
    void stateChanged();
    void positionChanged();
    void runningChanged();
    void programChanged();
    void progressChanged();
    void consoleChanged();
    void probingChanged();
    void lastProbeZChanged();
    void heightMapChanged();
    void autoLevelChanged();
    void interpolationsChanged();

private slots:
    void onReadyRead();
    void onErrorOccurred(QSerialPort::SerialPortError error);
    void pollStatus();

private:
    enum class Probe { Idle, Zeroing, Mapping };
    struct ProbePt { int col; int row; double x; double y; };

    void writeRealtime(char byte);
    void enqueue(const QString &line);   // queue a line for flow-controlled sending
    void pump();                          // send queued lines while the RX buffer has room
    void handleLine(const QString &line);
    void parseStatus(const QString &report);
    void parseProbe(const QString &report);
    void appendConsole(const QString &text);
    void finishRun(const QString &reason);

    // Probing helpers.
    void probeNextPoint();
    void finishHeightMap();
    void onProbeIdle();
    void abortProbe(const QString &reason);
    void rebuildProgram();

    static constexpr int RX_BUFFER = 128; // grblHAL serial RX buffer (bytes)

    QSerialPort m_port;
    QTimer m_statusTimer;
    QStringList m_availablePorts;
    QByteArray m_rx;

    QString m_state = QStringLiteral("Unknown");
    double m_mpos[3] = {0, 0, 0};
    double m_wpos[3] = {0, 0, 0};
    double m_wco[3]  = {0, 0, 0};
    double m_feed = 0;
    int m_feedOv = 100;

    // Outgoing flow control (shared by manual + program + probe lines, FIFO acks).
    QStringList m_queue;      // lines waiting to be written
    QList<int>  m_pending;    // byte counts of written-but-unacked lines

    // Program state.
    QStringList m_rawProgram; // as loaded from file
    QStringList m_program;    // streamed (== raw, or auto-levelled)
    QString m_programName;
    int m_acked = 0;          // program lines acknowledged (only while running)
    bool m_running = false;

    // Probing / height map.
    Probe m_probe = Probe::Idle;
    int m_zeroStage = 0;      // 0: awaiting contact, 1: applying zero/retract
    double m_pFeed = 50, m_pDepth = 20, m_pClear = 5, m_pThick = 0;
    QVector<ProbePt> m_order; // probe visiting order (snake)
    int m_ptIndex = 0;
    double m_lastProbeZ = 0;
    HeightMap m_map;

    // Auto-level. The interpolation segment length is derived from the probe
    // spacing divided by m_interpolations (default: 5 mm / 5 = 1 mm steps).
    bool m_autoLevel = false;
    int m_interpolations = 5; // sub-segments per probe cell when subdividing G1 moves
};
