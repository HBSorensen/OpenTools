#pragma once

// Camera-based fiducial alignment for double-sided registration.
//
// A spindle-mounted USB camera. For each of the 3 alignment holes the aligner
// auto-jogs the machine to the expected (mirrored) coordinate, detects the hole
// in the camera image, iteratively centers on it, and records the true machine
// position. Three correspondences yield a rigid transform (rotation+translation)
// applied to the bottom-side program.
//
// OpenCV is compiled in only when GSENDER_HAVE_OPENCV is defined (set by CMake
// when OpenCV is found); otherwise the aligner reports unavailable and the rest
// of the app still builds/runs (e.g. host CAM testing).

#include <QObject>
#include <QImage>
#include <QMutex>
#include <QVariantList>
#include <QVector>
#include <QPointF>
#include <QElapsedTimer>
#include <QQuickImageProvider>
#include <atomic>
#include <thread>

#ifdef GSENDER_HAVE_OPENCV
#include <opencv2/core.hpp>
#include <opencv2/videoio.hpp>
#endif

class GrblController;

// Holds the latest camera frame (with detection overlay) for QML display.
class FrameProvider : public QQuickImageProvider
{
public:
    FrameProvider() : QQuickImageProvider(QQuickImageProvider::Image) {}
    QImage requestImage(const QString &id, QSize *size, const QSize &req) override;
    void setFrame(const QImage &img);
private:
    QMutex m_mutex;
    QImage m_img;
};

class CameraAligner : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool available READ available NOTIFY availableChanged)
    Q_PROPERTY(bool cameraOpen READ cameraOpen NOTIFY cameraOpenChanged)
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(int frameCounter READ frameCounter NOTIFY frameChanged)
    Q_PROPERTY(double mmPerPixel READ mmPerPixel WRITE setMmPerPixel NOTIFY paramsChanged)
    Q_PROPERTY(int flipX READ flipX WRITE setFlipX NOTIFY paramsChanged)
    Q_PROPERTY(int flipY READ flipY WRITE setFlipY NOTIFY paramsChanged)
    Q_PROPERTY(int minRadius READ minRadius WRITE setMinRadius NOTIFY paramsChanged)
    Q_PROPERTY(int maxRadius READ maxRadius WRITE setMaxRadius NOTIFY paramsChanged)

    // Fisheye lens calibration.
    Q_PROPERTY(bool calibrated READ calibrated NOTIFY calibrationChanged)
    Q_PROPERTY(int calibViews READ calibViews NOTIFY calibrationChanged)
    Q_PROPERTY(double calibError READ calibError NOTIFY calibrationChanged)
    Q_PROPERTY(bool undistort READ undistort WRITE setUndistort NOTIFY calibrationChanged)
    Q_PROPERTY(int boardCols READ boardCols WRITE setBoardCols NOTIFY paramsChanged)
    Q_PROPERTY(int boardRows READ boardRows WRITE setBoardRows NOTIFY paramsChanged)
    Q_PROPERTY(double squareSize READ squareSize WRITE setSquareSize NOTIFY paramsChanged)

public:
    explicit CameraAligner(QObject *parent = nullptr);
    ~CameraAligner() override;

    void setController(GrblController *c) { m_ctrl = c; }
    void setFrameProvider(FrameProvider *p) { m_provider = p; }

    bool available() const;
    bool cameraOpen() const { return m_cameraOpen; }
    bool running() const { return m_state != St::Idle && m_state != St::Done && m_state != St::Fail; }
    QString status() const { return m_status; }
    int frameCounter() const { return m_frameCounter; }
    double mmPerPixel() const { return m_mmPerPixel; }
    int flipX() const { return m_flipX; }
    int flipY() const { return m_flipY; }
    int minRadius() const { return m_minRadius; }
    int maxRadius() const { return m_maxRadius; }
    void setMmPerPixel(double v) { m_mmPerPixel = v; emit paramsChanged(); }
    void setFlipX(int v) { m_flipX = v < 0 ? -1 : 1; emit paramsChanged(); }
    void setFlipY(int v) { m_flipY = v < 0 ? -1 : 1; emit paramsChanged(); }
    void setMinRadius(int v) { m_minRadius = v; emit paramsChanged(); }
    void setMaxRadius(int v) { m_maxRadius = v; emit paramsChanged(); }

    bool calibrated() const { return m_calibrated; }
    int calibViews() const { return m_calibViews; }
    double calibError() const { return m_calibErr; }
    bool undistort() const { return m_undistort; }
    void setUndistort(bool v);
    int boardCols() const { return m_boardCols; }
    int boardRows() const { return m_boardRows; }
    double squareSize() const { return m_squareSize; }
    void setBoardCols(int v) { m_boardCols = v; emit paramsChanged(); }
    void setBoardRows(int v) { m_boardRows = v; emit paramsChanged(); }
    void setSquareSize(double v) { m_squareSize = v; emit paramsChanged(); }

    Q_INVOKABLE bool openCamera(int index = 0);
    Q_INVOKABLE void closeCamera();
    // expected: list of [x,y] (or {x,y}) mirrored bottom-side hole coords (>=3).
    Q_INVOKABLE void beginAlignment(const QVariantList &expected);
    Q_INVOKABLE void cancel();

    // Fisheye calibration: capture a chessboard view (raw frame), then calibrate.
    Q_INVOKABLE bool captureCalibrationView();
    Q_INVOKABLE bool calibrate();
    Q_INVOKABLE void clearCalibration();

signals:
    void availableChanged();
    void cameraOpenChanged();
    void runningChanged();
    void statusChanged();
    void frameChanged();
    void paramsChanged();
    void calibrationChanged();
    void solved(double c, double s, double tx, double ty);
    void finished(bool ok);

private:
    enum class St { Idle, Move, SettleMove, Detect, Center, SettleJog, Record, Solve, Done, Fail };

    void setStatus(const QString &s);
    void setState(St s);
    void tick();                          // state-machine step (QTimer)
    void captureLoop();                   // background frame grabber
    bool grabLatest();                    // copy latest raw frame (main thread)
    void buildView();                     // m_view = undistort(m_work) or m_work
    bool detectHole(double &dxPx, double &dyPx, double &rPx); // offset from image center
    void publishFrame(double dxPx, double dyPx, double rPx, bool found);
    bool machineIdle() const;
    void moveAbs(double x, double y);
    void updateUndistortMaps();
    void saveCalibration();
    bool loadCalibration();
    QString calibPath() const;

    GrblController *m_ctrl = nullptr;
    FrameProvider *m_provider = nullptr;

    St m_state = St::Idle;
    QString m_status;
    int m_frameCounter = 0;

    // Alignment job.
    QVector<QPointF> m_expected, m_actual;
    int m_hole = 0;
    int m_iter = 0;
    int m_maxIter = 10;
    double m_tolMm = 0.02;
    QElapsedTimer m_settle;

    // Pixel<->mm mapping (tunable on device).
    double m_mmPerPixel = 0.05;
    int m_flipX = 1, m_flipY = -1;
    int m_minRadius = 5, m_maxRadius = 60;

    // Fisheye calibration state.
    bool m_calibrated = false;
    bool m_undistort = true;
    int m_calibViews = 0;
    double m_calibErr = 0.0;
    int m_boardCols = 9, m_boardRows = 6;   // inner corners
    double m_squareSize = 10.0;             // mm (scale only; not needed for undistort)

    // Camera + capture thread.
    bool m_cameraOpen = false;
    std::atomic<bool> m_grabbing{false};
    std::thread m_thread;
    QMutex m_frameMutex;
#ifdef GSENDER_HAVE_OPENCV
    cv::VideoCapture m_cap;
    cv::Mat m_shared;     // written by capture thread (raw)
    cv::Mat m_work;       // main-thread raw copy (used for chessboard capture)
    cv::Mat m_view;       // undistorted (or raw) frame used for detection + preview
    cv::Mat m_K, m_D;     // fisheye intrinsics + distortion
    cv::Mat m_map1, m_map2;
    cv::Size m_calibImgSize;   // image size used for calibration
    cv::Size m_mapSize;        // size the remap maps were built for
    std::vector<std::vector<cv::Point2f>> m_imgPoints;
#endif
    class QTimer *m_timer = nullptr;
};
