#include "Geometry.h"

#include <algorithm>
#include <cmath>

using namespace Clipper2Lib;

namespace geo {

Path64 circle(double cx, double cy, double r, int segs)
{
    Path64 p;
    if (segs < 8) segs = 8;
    p.reserve(segs);
    for (int i = 0; i < segs; ++i) {
        double a = 2.0 * M_PI * i / segs;
        p.push_back(Point64(I(cx + r * std::cos(a)), I(cy + r * std::sin(a))));
    }
    return p;
}

Path64 rectCentered(double cx, double cy, double w, double h)
{
    double hw = w / 2, hh = h / 2;
    return Path64{
        Point64(I(cx - hw), I(cy - hh)),
        Point64(I(cx + hw), I(cy - hh)),
        Point64(I(cx + hw), I(cy + hh)),
        Point64(I(cx - hw), I(cy + hh)),
    };
}

Path64 obround(double cx, double cy, double w, double h)
{
    // Stadium shape: rectangle with the short pair of ends replaced by semicircles.
    Path64 p;
    const int segs = 16;
    if (w >= h) {
        double r = h / 2, dx = (w - h) / 2;
        // right cap
        for (int i = 0; i <= segs; ++i) {
            double a = -M_PI / 2 + M_PI * i / segs;
            p.push_back(Point64(I(cx + dx + r * std::cos(a)), I(cy + r * std::sin(a))));
        }
        // left cap
        for (int i = 0; i <= segs; ++i) {
            double a = M_PI / 2 + M_PI * i / segs;
            p.push_back(Point64(I(cx - dx + r * std::cos(a)), I(cy + r * std::sin(a))));
        }
    } else {
        double r = w / 2, dy = (h - w) / 2;
        for (int i = 0; i <= segs; ++i) {
            double a = 0 + M_PI * i / segs;
            p.push_back(Point64(I(cx + r * std::cos(a)), I(cy + dy + r * std::sin(a))));
        }
        for (int i = 0; i <= segs; ++i) {
            double a = M_PI + M_PI * i / segs;
            p.push_back(Point64(I(cx + r * std::cos(a)), I(cy - dy + r * std::sin(a))));
        }
    }
    return p;
}

Path64 regularPolygon(double cx, double cy, double dia, int verts, double rotDeg)
{
    Path64 p;
    if (verts < 3) verts = 3;
    double r = dia / 2, rot = rotDeg * M_PI / 180.0;
    for (int i = 0; i < verts; ++i) {
        double a = rot + 2.0 * M_PI * i / verts;
        p.push_back(Point64(I(cx + r * std::cos(a)), I(cy + r * std::sin(a))));
    }
    return p;
}

void appendArc(Path64 &out, double x0, double y0, double x1, double y1,
               double cx, double cy, bool ccw, double stepDeg)
{
    double r = std::hypot(x0 - cx, y0 - cy);
    double a0 = std::atan2(y0 - cy, x0 - cx);
    double a1 = std::atan2(y1 - cy, x1 - cx);
    double sweep = a1 - a0;
    // Normalize sweep into the chosen direction.
    if (ccw) { while (sweep <= 0) sweep += 2 * M_PI; }
    else     { while (sweep >= 0) sweep -= 2 * M_PI; }
    // Full circle when start ≈ end (common for round pads drawn as arcs).
    if (std::abs(sweep) < 1e-9)
        sweep = ccw ? 2 * M_PI : -2 * M_PI;

    int steps = std::max(1, int(std::ceil(std::abs(sweep) / (stepDeg * M_PI / 180.0))));
    for (int i = 1; i <= steps; ++i) {
        double a = a0 + sweep * i / steps;
        out.push_back(Point64(I(cx + r * std::cos(a)), I(cy + r * std::sin(a))));
    }
}

void Bounds::add(double x, double y)
{
    if (!valid) { minX = maxX = x; minY = maxY = y; valid = true; return; }
    minX = std::min(minX, x); maxX = std::max(maxX, x);
    minY = std::min(minY, y); maxY = std::max(maxY, y);
}

Bounds bounds(const Paths64 &paths)
{
    Bounds b;
    for (const auto &path : paths)
        for (const auto &pt : path)
            b.add(MM(pt.x), MM(pt.y));
    return b;
}

double Rigid::rotationDeg() const { return std::atan2(s, c) * 180.0 / M_PI; }

Rigid solveRigid(const QVector<QPointF> &from, const QVector<QPointF> &to)
{
    Rigid r;
    int n = std::min(from.size(), to.size());
    if (n < 2)
        return r;

    // Centroids.
    double fcx = 0, fcy = 0, tcx = 0, tcy = 0;
    for (int i = 0; i < n; ++i) {
        fcx += from[i].x(); fcy += from[i].y();
        tcx += to[i].x();   tcy += to[i].y();
    }
    fcx /= n; fcy /= n; tcx /= n; tcy /= n;

    // Optimal 2D rotation: theta = atan2(Σ cross, Σ dot) over centered points.
    double sumDot = 0, sumCross = 0;
    for (int i = 0; i < n; ++i) {
        double fx = from[i].x() - fcx, fy = from[i].y() - fcy;
        double tx = to[i].x() - tcx,   ty = to[i].y() - tcy;
        sumDot   += fx * tx + fy * ty;
        sumCross += fx * ty - fy * tx;
    }
    double theta = std::atan2(sumCross, sumDot);
    r.c = std::cos(theta);
    r.s = std::sin(theta);
    // Translation so the centroid maps correctly.
    r.tx = tcx - (r.c * fcx - r.s * fcy);
    r.ty = tcy - (r.s * fcx + r.c * fcy);
    return r;
}

} // namespace geo
