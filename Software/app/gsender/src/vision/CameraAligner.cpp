#include "CameraAligner.h"
#include "GrblController.h"
#include "pcb/Geometry.h"

#include <QTimer>
#include <QVariant>
#include <QDir>
#include <QProcessEnvironment>
#include <cmath>

#ifdef GSENDER_HAVE_OPENCV
#include <opencv2/imgproc.hpp>
#include <opencv2/calib3d.hpp>
#endif

// --- FrameProvider -----------------------------------------------------------

QImage FrameProvider::requestImage(const QString &, QSize *size, const QSize &)
{
    QMutexLocker lock(&m_mutex);
    if (size) *size = m_img.size();
    return m_img;
}

void FrameProvider::setFrame(const QImage &img)
{
    QMutexLocker lock(&m_mutex);
    m_img = img;
}

// --- CameraAligner -----------------------------------------------------------

CameraAligner::CameraAligner(QObject *parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(120);
    connect(m_timer, &QTimer::timeout, this, &CameraAligner::tick);
    loadCalibration();   // restore a previous fisheye calibration if present
}

CameraAligner::~CameraAligner()
{
    closeCamera();
}

bool CameraAligner::available() const
{
#ifdef GSENDER_HAVE_OPENCV
    return true;
#else
    return false;
#endif
}

void CameraAligner::setStatus(const QString &s)
{
    if (m_status == s) return;
    m_status = s;
    emit statusChanged();
}

void CameraAligner::setState(St s)
{
    bool wasRunning = running();
    m_state = s;
    if (running() != wasRunning)
        emit runningChanged();
}

bool CameraAligner::machineIdle() const
{
    return m_ctrl && m_ctrl->connected() && m_ctrl->state() == QLatin1String("Idle");
}

void CameraAligner::moveAbs(double x, double y)
{
    if (m_ctrl)
        m_ctrl->sendCommand(QString("G0 X%1 Y%2")
                                .arg(x, 0, 'f', 3).arg(y, 0, 'f', 3));
}

bool CameraAligner::openCamera(int index)
{
#ifdef GSENDER_HAVE_OPENCV
    if (m_cameraOpen) return true;
    if (!m_cap.open(index, cv::CAP_V4L2) && !m_cap.open(index)) {
        setStatus("Failed to open camera");
        return false;
    }
    m_cap.set(cv::CAP_PROP_FRAME_WIDTH, 640);
    m_cap.set(cv::CAP_PROP_FRAME_HEIGHT, 480);
    m_grabbing = true;
    m_thread = std::thread(&CameraAligner::captureLoop, this);
    m_cameraOpen = true;
    emit cameraOpenChanged();
    m_timer->start();
    setStatus("Camera open");
    return true;
#else
    Q_UNUSED(index);
    setStatus("Vision unavailable (built without OpenCV)");
    return false;
#endif
}

void CameraAligner::closeCamera()
{
    m_grabbing = false;
    if (m_thread.joinable())
        m_thread.join();
    if (m_timer) m_timer->stop();
#ifdef GSENDER_HAVE_OPENCV
    if (m_cap.isOpened())
        m_cap.release();
#endif
    if (m_cameraOpen) {
        m_cameraOpen = false;
        emit cameraOpenChanged();
    }
}

void CameraAligner::captureLoop()
{
#ifdef GSENDER_HAVE_OPENCV
    while (m_grabbing) {
        cv::Mat f;
        if (m_cap.read(f) && !f.empty()) {
            QMutexLocker lock(&m_frameMutex);
            f.copyTo(m_shared);
        }
    }
#endif
}

bool CameraAligner::grabLatest()
{
#ifdef GSENDER_HAVE_OPENCV
    QMutexLocker lock(&m_frameMutex);
    if (m_shared.empty()) return false;
    m_shared.copyTo(m_work);       // raw frame (used for chessboard capture)
    return true;
#else
    return false;
#endif
}

void CameraAligner::buildView()
{
#ifdef GSENDER_HAVE_OPENCV
    if (m_work.empty()) return;
    if (m_calibrated && m_undistort && !m_K.empty()) {
        if (m_map1.empty() || m_mapSize != m_work.size())
            updateUndistortMaps();
        cv::remap(m_work, m_view, m_map1, m_map2, cv::INTER_LINEAR);
    } else {
        m_view = m_work;           // shallow ref; publishFrame clones before drawing
    }
#endif
}

void CameraAligner::updateUndistortMaps()
{
#ifdef GSENDER_HAVE_OPENCV
    if (m_K.empty() || m_work.empty()) return;
    const cv::Size sz = m_work.size();
    cv::Mat newK;
    // Keep the full field of view (balance=0) at the native resolution.
    cv::fisheye::estimateNewCameraMatrixForUndistortRectify(
        m_K, m_D, sz, cv::Matx33d::eye(), newK, 0.0, sz, 1.0);
    cv::fisheye::initUndistortRectifyMap(
        m_K, m_D, cv::Matx33d::eye(), newK, sz, CV_16SC2, m_map1, m_map2);
    m_mapSize = sz;
#endif
}

bool CameraAligner::detectHole(double &dxPx, double &dyPx, double &rPx)
{
#ifdef GSENDER_HAVE_OPENCV
    if (m_view.empty()) return false;
    cv::Mat gray;
    cv::cvtColor(m_view, gray, cv::COLOR_BGR2GRAY);
    cv::GaussianBlur(gray, gray, cv::Size(5, 5), 1.5);

    std::vector<cv::Vec3f> circles;
    cv::HoughCircles(gray, circles, cv::HOUGH_GRADIENT, 1.0,
                     double(gray.rows) / 8.0, 100, 30, m_minRadius, m_maxRadius);
    if (circles.empty()) return false;

    // Pick the circle nearest the image centre.
    const double cxImg = gray.cols / 2.0, cyImg = gray.rows / 2.0;
    int best = 0; double bestD = 1e18;
    for (size_t i = 0; i < circles.size(); ++i) {
        double d = std::hypot(circles[i][0] - cxImg, circles[i][1] - cyImg);
        if (d < bestD) { bestD = d; best = int(i); }
    }
    dxPx = circles[best][0] - cxImg;
    dyPx = circles[best][1] - cyImg;
    rPx  = circles[best][2];
    return true;
#else
    Q_UNUSED(dxPx); Q_UNUSED(dyPx); Q_UNUSED(rPx);
    return false;
#endif
}

void CameraAligner::publishFrame(double dxPx, double dyPx, double rPx, bool found)
{
#ifdef GSENDER_HAVE_OPENCV
    if (m_view.empty() || !m_provider) return;
    cv::Mat vis = m_view.clone();
    const int w = vis.cols, h = vis.rows;
    // Crosshair at image centre.
    cv::line(vis, {w / 2 - 15, h / 2}, {w / 2 + 15, h / 2}, {0, 255, 0}, 1);
    cv::line(vis, {w / 2, h / 2 - 15}, {w / 2, h / 2 + 15}, {0, 255, 0}, 1);
    if (found) {
        cv::Point c(int(w / 2 + dxPx), int(h / 2 + dyPx));
        cv::circle(vis, c, int(rPx), {0, 0, 255}, 2);
        cv::circle(vis, c, 2, {0, 0, 255}, -1);
    }
    cv::Mat rgb;
    cv::cvtColor(vis, rgb, cv::COLOR_BGR2RGB);
    QImage img(rgb.data, rgb.cols, rgb.rows, int(rgb.step), QImage::Format_RGB888);
    m_provider->setFrame(img.copy());
    ++m_frameCounter;
    emit frameChanged();
#else
    Q_UNUSED(dxPx); Q_UNUSED(dyPx); Q_UNUSED(rPx); Q_UNUSED(found);
#endif
}

void CameraAligner::beginAlignment(const QVariantList &expected)
{
    if (!available()) { setStatus("Vision unavailable (built without OpenCV)"); emit finished(false); return; }
    if (!m_cameraOpen) { setStatus("Open the camera first"); emit finished(false); return; }
    if (!m_ctrl || !m_ctrl->connected()) { setStatus("Not connected to controller"); emit finished(false); return; }
    if (expected.size() < 3) { setStatus("Need 3 alignment holes"); emit finished(false); return; }

    m_expected.clear();
    m_actual.clear();
    for (const QVariant &v : expected) {
        double x = 0, y = 0;
        if (v.canConvert<QVariantList>()) {
            const QVariantList p = v.toList();
            if (p.size() >= 2) { x = p[0].toDouble(); y = p[1].toDouble(); }
        } else {
            const QVariantMap m = v.toMap();
            x = m.value("x").toDouble(); y = m.value("y").toDouble();
        }
        m_expected.push_back(QPointF(x, y));
    }
    m_hole = 0;
    m_iter = 0;
    setStatus("Aligning: hole 1");
    setState(St::Move);
}

void CameraAligner::cancel()
{
    if (running()) {
        setStatus("Alignment cancelled");
        setState(St::Fail);
        emit finished(false);
    }
}

void CameraAligner::tick()
{
    // Refresh the preview whenever the camera is open; run the (heavier) hole
    // detection only while an alignment is actually in progress.
    double dx = 0, dy = 0, r = 0;
    bool found = false;
    if (grabLatest()) {
        buildView();
        if (running())
            found = detectHole(dx, dy, r);
        publishFrame(dx, dy, r, found);
    }

    if (!running())
        return;

    switch (m_state) {
    case St::Move:
        if (m_hole >= m_expected.size()) { setState(St::Solve); break; }
        setStatus(QString("Hole %1/%2: moving to expected position")
                      .arg(m_hole + 1).arg(m_expected.size()));
        moveAbs(m_expected[m_hole].x(), m_expected[m_hole].y());
        m_iter = 0;
        m_settle.restart();
        setState(St::SettleMove);
        break;

    case St::SettleMove:
        if (machineIdle() && m_settle.elapsed() > 400)
            setState(St::Detect);
        break;

    case St::Detect:
        if (!found) {
            if (++m_iter > m_maxIter) {
                setStatus(QString("Hole %1: not found — check lighting / radius range")
                              .arg(m_hole + 1));
                setState(St::Fail);
                emit finished(false);
            }
            break; // try again next frame
        }
        {
            double dxmm = dx * m_mmPerPixel;
            double dymm = dy * m_mmPerPixel;
            double errmm = std::hypot(dxmm, dymm);
            if (errmm <= m_tolMm || m_iter >= m_maxIter) {
                setState(St::Record);
            } else {
                double tx = m_ctrl->wx() + m_flipX * dxmm;
                double ty = m_ctrl->wy() + m_flipY * dymm;
                moveAbs(tx, ty);
                ++m_iter;
                m_settle.restart();
                setState(St::SettleJog);
            }
        }
        break;

    case St::SettleJog:
        if (machineIdle() && m_settle.elapsed() > 300)
            setState(St::Detect);
        break;

    case St::Record:
        m_actual.push_back(QPointF(m_ctrl->wx(), m_ctrl->wy()));
        setStatus(QString("Hole %1 centered at (%2, %3)")
                      .arg(m_hole + 1)
                      .arg(m_ctrl->wx(), 0, 'f', 3).arg(m_ctrl->wy(), 0, 'f', 3));
        ++m_hole;
        setState(m_hole >= m_expected.size() ? St::Solve : St::Move);
        break;

    case St::Solve: {
        geo::Rigid t = geo::solveRigid(m_expected, m_actual);
        setStatus(QString("Aligned: rotation %1°, offset (%2, %3) mm")
                      .arg(t.rotationDeg(), 0, 'f', 3)
                      .arg(t.tx, 0, 'f', 3).arg(t.ty, 0, 'f', 3));
        emit solved(t.c, t.s, t.tx, t.ty);
        setState(St::Done);
        emit finished(true);
        break;
    }

    default:
        break;
    }
}

// --- Fisheye calibration -----------------------------------------------------

void CameraAligner::setUndistort(bool v)
{
    if (m_undistort == v) return;
    m_undistort = v;
#ifdef GSENDER_HAVE_OPENCV
    m_map1.release();     // rebuilt lazily by buildView()
#endif
    emit calibrationChanged();
}

QString CameraAligner::calibPath() const
{
    const QString dir = QProcessEnvironment::systemEnvironment()
                            .value("GSENDER_DATA", "/data");
    QDir().mkpath(dir);
    return QDir(dir).absoluteFilePath("camera_calib.yml");
}

bool CameraAligner::captureCalibrationView()
{
#ifdef GSENDER_HAVE_OPENCV
    if (!m_cameraOpen) { setStatus("Open the camera first"); return false; }
    if (!grabLatest() || m_work.empty()) { setStatus("No camera frame"); return false; }

    cv::Mat gray;
    cv::cvtColor(m_work, gray, cv::COLOR_BGR2GRAY);
    std::vector<cv::Point2f> corners;
    const cv::Size board(m_boardCols, m_boardRows);
    bool found = cv::findChessboardCorners(
        gray, board, corners,
        cv::CALIB_CB_ADAPTIVE_THRESH | cv::CALIB_CB_NORMALIZE_IMAGE | cv::CALIB_CB_FAST_CHECK);
    if (!found) {
        setStatus(QString("Chessboard (%1x%2 inner corners) not found — reposition / lighting")
                      .arg(m_boardCols).arg(m_boardRows));
        return false;
    }
    cv::cornerSubPix(gray, corners, cv::Size(11, 11), cv::Size(-1, -1),
                     cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 30, 0.1));
    m_imgPoints.push_back(corners);
    m_calibImgSize = m_work.size();
    m_calibViews = int(m_imgPoints.size());
    setStatus(QString("Captured chessboard view %1 (need >=6 to calibrate)").arg(m_calibViews));
    emit calibrationChanged();
    return true;
#else
    setStatus("Vision unavailable (built without OpenCV)");
    return false;
#endif
}

bool CameraAligner::calibrate()
{
#ifdef GSENDER_HAVE_OPENCV
    const int need = 6;
    if (int(m_imgPoints.size()) < need) {
        setStatus(QString("Need >=%1 chessboard views (have %2)").arg(need).arg(m_imgPoints.size()));
        return false;
    }
    // Object points: the chessboard grid, Z=0, in mm (scale only).
    std::vector<cv::Point3f> grid;
    for (int r = 0; r < m_boardRows; ++r)
        for (int c = 0; c < m_boardCols; ++c)
            grid.emplace_back(float(c * m_squareSize), float(r * m_squareSize), 0.f);
    std::vector<std::vector<cv::Point3f>> objPoints(m_imgPoints.size(), grid);

    cv::Mat K, D;
    std::vector<cv::Mat> rvecs, tvecs;
    int flags = cv::fisheye::CALIB_RECOMPUTE_EXTRINSIC | cv::fisheye::CALIB_FIX_SKEW;
    try {
        double rms = cv::fisheye::calibrate(
            objPoints, m_imgPoints, m_calibImgSize, K, D, rvecs, tvecs, flags,
            cv::TermCriteria(cv::TermCriteria::EPS + cv::TermCriteria::MAX_ITER, 100, 1e-6));
        m_K = K; m_D = D; m_calibErr = rms; m_calibrated = true;
        m_map1.release();
        saveCalibration();
        setStatus(QString("Fisheye calibrated: RMS %1 px over %2 views")
                      .arg(rms, 0, 'f', 3).arg(m_imgPoints.size()));
        emit calibrationChanged();
        return true;
    } catch (const cv::Exception &e) {
        setStatus(QString("Calibration failed: %1").arg(e.what()));
        return false;
    }
#else
    setStatus("Vision unavailable (built without OpenCV)");
    return false;
#endif
}

void CameraAligner::clearCalibration()
{
    m_calibrated = false;
    m_calibViews = 0;
    m_calibErr = 0.0;
#ifdef GSENDER_HAVE_OPENCV
    m_imgPoints.clear();
    m_K.release(); m_D.release();
    m_map1.release(); m_map2.release();
#endif
    setStatus("Calibration cleared");
    emit calibrationChanged();
}

void CameraAligner::saveCalibration()
{
#ifdef GSENDER_HAVE_OPENCV
    cv::FileStorage fs(calibPath().toStdString(), cv::FileStorage::WRITE);
    if (!fs.isOpened()) return;
    fs << "model" << "fisheye"
       << "K" << m_K << "D" << m_D
       << "w" << m_calibImgSize.width << "h" << m_calibImgSize.height
       << "rms" << m_calibErr
       << "boardCols" << m_boardCols << "boardRows" << m_boardRows
       << "square" << m_squareSize;
    fs.release();
#endif
}

bool CameraAligner::loadCalibration()
{
#ifdef GSENDER_HAVE_OPENCV
    cv::FileStorage fs(calibPath().toStdString(), cv::FileStorage::READ);
    if (!fs.isOpened()) return false;
    fs["K"] >> m_K;
    fs["D"] >> m_D;
    int w = 0, h = 0;
    fs["w"] >> w; fs["h"] >> h; fs["rms"] >> m_calibErr;
    fs.release();
    if (m_K.empty() || m_D.empty()) return false;
    m_calibImgSize = cv::Size(w, h);
    m_calibrated = true;
    m_map1.release();
    emit calibrationChanged();
    return true;
#else
    return false;
#endif
}
