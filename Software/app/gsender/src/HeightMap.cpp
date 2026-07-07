#include "HeightMap.h"

#include <QRegularExpression>
#include <QHash>
#include <QtGlobal>
#include <algorithm>
#include <cmath>

void HeightMap::configure(double x0, double y0, double width, double height,
                          int cols, int rows)
{
    m_cols = std::max(2, cols);
    m_rows = std::max(2, rows);
    m_x0 = x0;
    m_y0 = y0;
    m_dx = width / (m_cols - 1);
    m_dy = height / (m_rows - 1);
    m_z.fill(0.0, m_cols * m_rows);
    m_ref = 0;
    m_min = m_max = 0;
    m_valid = false;
}

void HeightMap::clear()
{
    m_valid = false;
    m_cols = m_rows = 0;
    m_z.clear();
    m_ref = m_min = m_max = 0;
}

void HeightMap::setZ(int col, int row, double z)
{
    if (col < 0 || col >= m_cols || row < 0 || row >= m_rows)
        return;
    m_z[index(col, row)] = z;
}

double HeightMap::at(int col, int row) const
{
    if (col < 0 || col >= m_cols || row < 0 || row >= m_rows || m_z.isEmpty())
        return 0.0;
    return m_z[index(col, row)] - m_ref;
}

void HeightMap::finalize()
{
    if (m_z.isEmpty()) {
        m_valid = false;
        return;
    }
    m_ref = m_z[index(0, 0)];          // origin is the Z-zero reference
    m_min = m_max = 0.0;
    for (double raw : std::as_const(m_z)) {
        const double v = raw - m_ref;
        m_min = std::min(m_min, v);
        m_max = std::max(m_max, v);
    }
    m_valid = true;
}

double HeightMap::interp(double x, double y) const
{
    if (!m_valid || m_cols < 2 || m_rows < 2)
        return 0.0;

    // Clamp into the grid and find the containing cell.
    double fx = (m_dx != 0.0) ? (x - m_x0) / m_dx : 0.0;
    double fy = (m_dy != 0.0) ? (y - m_y0) / m_dy : 0.0;
    fx = std::clamp(fx, 0.0, double(m_cols - 1));
    fy = std::clamp(fy, 0.0, double(m_rows - 1));

    int c0 = std::min(int(std::floor(fx)), m_cols - 2);
    int r0 = std::min(int(std::floor(fy)), m_rows - 2);
    double tx = fx - c0;
    double ty = fy - r0;

    // Bilinear interpolation over normalized corner values.
    double z00 = at(c0,     r0);
    double z10 = at(c0 + 1, r0);
    double z01 = at(c0,     r0 + 1);
    double z11 = at(c0 + 1, r0 + 1);
    double a = z00 * (1 - tx) + z10 * tx;
    double b = z01 * (1 - tx) + z11 * tx;
    return a * (1 - ty) + b * ty;
}

// --- G-code auto-level transform --------------------------------------------

namespace {

// Extract the numeric argument for a G-code word letter (case-insensitive),
// e.g. word 'X' from "G1 X12.5 Y-3". Returns false if the letter is absent.
bool word(const QString &line, QChar letter, double &out)
{
    static QHash<QChar, QRegularExpression> cache;
    auto it = cache.find(letter);
    if (it == cache.end()) {
        QRegularExpression re(QStringLiteral("(?i)%1\\s*([-+]?\\d*\\.?\\d+)")
                                  .arg(letter));
        it = cache.insert(letter, re);
    }
    const QRegularExpressionMatch m = it->match(line);
    if (!m.hasMatch())
        return false;
    out = m.captured(1).toDouble();
    return true;
}

QString fmt(double v)
{
    return QString::number(v, 'f', 3);
}

} // namespace

QStringList applyHeightMap(const QStringList &src, const HeightMap &map,
                           double segLen, QString &warning)
{
    warning.clear();
    if (!map.valid())
        return src;
    if (segLen <= 0.0)
        segLen = 2.0;

    // Quick unsupported-mode scan (relative positioning / inch units).
    for (const QString &line : src) {
        const QString up = line.toUpper();
        if (up.contains(QRegularExpression("G0*91(?![0-9])"))
            || up.contains(QRegularExpression("G0*20(?![0-9])"))) {
            warning = QStringLiteral(
                "Program uses relative (G91) or inch (G20) mode — auto-level "
                "skipped; streaming the original program.");
            return src;
        }
    }

    QStringList out;
    out.reserve(src.size() * 2);

    double cx = 0, cy = 0, cz = 0;   // current position (work coords, mm)
    int motion = -1;                  // active modal motion mode: 0/1/2/3
    bool warnedArc = false;

    QRegularExpression gcodeRe("(?i)G(\\d+\\.?\\d*)");

    for (const QString &raw : src) {
        const QString line = raw.trimmed();
        if (line.isEmpty())
            continue;

        // Update modal motion mode from any G0/G1/G2/G3 on the line.
        auto it = gcodeRe.globalMatch(line);
        while (it.hasNext()) {
            int code = int(it.next().captured(1).toDouble());
            if (code >= 0 && code <= 3)
                motion = code;
        }

        double x, y, z, f;
        const bool hasX = word(line, 'X', x);
        const bool hasY = word(line, 'Y', y);
        const bool hasZ = word(line, 'Z', z);
        const bool hasF = word(line, 'F', f);
        const bool hasMove = hasX || hasY || hasZ;

        // Non-motion lines (M-codes, comments, settings) pass through verbatim.
        if (!hasMove || motion < 0) {
            out << line;
            continue;
        }

        const double nx = hasX ? x : cx;
        const double ny = hasY ? y : cy;
        const double nz = hasZ ? z : cz;
        const QString fStr = hasF ? (QStringLiteral(" F") + fmt(f)) : QString();

        if (motion == 0) {
            // Rapid: leave untouched, just track position.
            out << line;
        } else if (motion == 1) {
            const double dxy = std::hypot(nx - cx, ny - cy);
            if (dxy < 1e-6) {
                // Pure plunge / Z move: offset the endpoint.
                out << QStringLiteral("G1 Z%1%2")
                           .arg(fmt(nz + map.interp(nx, ny)), fStr);
            } else {
                // Cutting move: subdivide in XY and follow the surface.
                int steps = std::max(1, int(std::ceil(dxy / segLen)));
                for (int k = 1; k <= steps; ++k) {
                    double t = double(k) / steps;
                    double ix = cx + (nx - cx) * t;
                    double iy = cy + (ny - cy) * t;
                    double iz = cz + (nz - cz) * t;
                    out << QStringLiteral("G1 X%1 Y%2 Z%3%4")
                               .arg(fmt(ix), fmt(iy),
                                    fmt(iz + map.interp(ix, iy)),
                                    k == 1 ? fStr : QString());
                }
            }
        } else {
            // Arc (G2/G3): pass through, offsetting the endpoint Z only.
            if (!warnedArc) {
                warning = QStringLiteral(
                    "Program contains arcs (G2/G3); only their endpoints are "
                    "height-corrected.");
                warnedArc = true;
            }
            if (hasZ) {
                QString mod = line;
                mod.replace(QRegularExpression("(?i)Z\\s*[-+]?\\d*\\.?\\d+"),
                            QStringLiteral("Z") + fmt(nz + map.interp(nx, ny)));
                out << mod;
            } else {
                out << line;
            }
        }

        cx = nx;
        cy = ny;
        cz = nz;
    }

    return out;
}
