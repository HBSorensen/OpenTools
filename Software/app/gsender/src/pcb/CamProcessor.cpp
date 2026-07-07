#include "CamProcessor.h"

#include <QRegularExpression>
#include <algorithm>
#include <cmath>

using namespace Clipper2Lib;

namespace {
QString n3(double v) { return QString::number(v, 'f', 3); }

// Emit a motion line with both X and Y so a later rigid transform can rotate it.
QString moveXY(const char *g, double x, double y, bool mirror, double axis)
{
    if (mirror) x = 2.0 * axis - x;
    return QString("%1 X%2 Y%3").arg(g, n3(x), n3(y));
}
}

double CamProcessor::effectiveWidth(const ToolParams &p)
{
    if (p.width > 0.0)
        return p.width;
    // Cut width of a V-bit at a given depth: tip flat + 2*depth*tan(halfAngle).
    double half = (p.tipAngle * 0.5) * M_PI / 180.0;
    return p.tipDia + 2.0 * p.cutDepth * std::tan(half);
}

QStringList CamProcessor::isolation(const Paths64 &copper, const ToolParams &p,
                                    bool mirror, double axisX)
{
    QStringList g;
    if (copper.empty())
        return g;

    const double w = effectiveWidth(p);
    const int passes = std::max(2, p.passes);
    const double step = w * (1.0 - std::clamp(p.overlap, 0.0, 0.9));

    g << "; gsender PCB isolation";
    g << QString("; tool width %1 mm, %2 passes, depth %3 mm%4")
             .arg(n3(w)).arg(passes).arg(n3(p.cutDepth),
                  mirror ? ", mirrored (bottom)" : "");
    g << "G21" << "G90" << "G17";
    if (p.spindleRpm > 0)
        g << QString("M3 S%1").arg(int(p.spindleRpm));
    g << QString("G0 Z%1").arg(n3(p.travelZ));

    for (int k = 0; k < passes; ++k) {
        const double delta = w / 2.0 + k * step;          // mm outward from copper
        Paths64 off = InflatePaths(copper, delta * geo::SCALE,
                                   JoinType::Round, EndType::Polygon,
                                   2.0, 0.005 * geo::SCALE);
        g << QString("; --- pass %1/%2 (offset %3 mm) ---")
                 .arg(k + 1).arg(passes).arg(n3(delta));
        for (const Path64 &path : off) {
            if (path.size() < 2)
                continue;
            const double x0 = geo::MM(path.front().x), y0 = geo::MM(path.front().y);
            g << QString("G0 Z%1").arg(n3(p.travelZ));
            g << moveXY("G0", x0, y0, mirror, axisX);
            g << QString("G1 Z%1 F%2").arg(n3(-p.cutDepth)).arg(int(p.plungeFeed));
            bool first = true;
            for (const Point64 &pt : path) {
                QString line = moveXY("G1", geo::MM(pt.x), geo::MM(pt.y), mirror, axisX);
                if (first) { line += QString(" F%1").arg(int(p.feed)); first = false; }
                g << line;
            }
            g << moveXY("G1", x0, y0, mirror, axisX);      // close the loop
            g << QString("G0 Z%1").arg(n3(p.safeZ));
        }
    }

    g << QString("G0 Z%1").arg(n3(p.travelZ));
    if (p.spindleRpm > 0)
        g << "M5";
    g << "M2";
    return g;
}

QStringList CamProcessor::drill(const QVector<ExcellonParser::Hole> &holes,
                                const ToolParams &p, bool mirror, double axisX)
{
    QStringList g;
    if (holes.isEmpty())
        return g;

    g << "; gsender PCB drilling";
    g << "G21" << "G90";
    if (p.spindleRpm > 0)
        g << QString("M3 S%1").arg(int(p.spindleRpm));
    g << QString("G0 Z%1").arg(n3(p.travelZ));

    for (const auto &h : holes) {
        g << moveXY("G0", h.x, h.y, mirror, axisX);
        g << QString("G1 Z%1 F%2").arg(n3(-p.drillDepth)).arg(int(p.drillFeed));
        g << QString("G0 Z%1").arg(n3(p.travelZ));
    }

    if (p.spindleRpm > 0)
        g << "M5";
    g << "M2";
    return g;
}

QStringList CamProcessor::applyTransform(const QStringList &gcode, const geo::Rigid &t)
{
    QStringList out;
    out.reserve(gcode.size());
    static const QRegularExpression reX(R"([Xx](-?\d+\.?\d*))");
    static const QRegularExpression reY(R"([Yy](-?\d+\.?\d*))");
    static const QRegularExpression reI(R"([Ii](-?\d+\.?\d*))");
    static const QRegularExpression reJ(R"([Jj](-?\d+\.?\d*))");

    for (const QString &line : gcode) {
        QRegularExpressionMatch mx = reX.match(line);
        QRegularExpressionMatch my = reY.match(line);
        if (!mx.hasMatch() || !my.hasMatch()) {
            out << line;               // no XY motion: pass through unchanged
            continue;
        }
        double x = mx.captured(1).toDouble();
        double y = my.captured(1).toDouble();
        t.map(x, y);

        QString l = line;
        l.replace(mx.captured(0), QString("X%1").arg(n3(x)));
        l.replace(my.captured(0), QString("Y%1").arg(n3(y)));

        // Rotate arc offsets (I/J) as a vector — rotation only, no translation.
        QRegularExpressionMatch mi = reI.match(line);
        QRegularExpressionMatch mj = reJ.match(line);
        if (mi.hasMatch() && mj.hasMatch()) {
            double i = mi.captured(1).toDouble();
            double j = mj.captured(1).toDouble();
            double ni = t.c * i - t.s * j;
            double nj = t.s * i + t.c * j;
            l.replace(mi.captured(0), QString("I%1").arg(n3(ni)));
            l.replace(mj.captured(0), QString("J%1").arg(n3(nj)));
        }
        out << l;
    }
    return out;
}
