#pragma once

// Geometry helpers bridging millimetres and Clipper2's integer coordinate space,
// aperture/primitive polygon builders, bounds, and a 2D rigid-transform solver
// (used by the camera alignment to map expected -> measured hole positions).

#include <QVector>
#include <QPointF>
#include <cstdint>

#include "clipper2/clipper.h"

namespace geo {

// Integer units per millimetre. 1e5 => 0.01 µm resolution, well within int64 for
// board coordinates up to hundreds of mm.
constexpr double SCALE = 100000.0;

inline int64_t I(double mm) { return static_cast<int64_t>(llround(mm * SCALE)); }
inline double  MM(int64_t v) { return static_cast<double>(v) / SCALE; }

Clipper2Lib::Path64 circle(double cx, double cy, double r, int segs = 48);
Clipper2Lib::Path64 rectCentered(double cx, double cy, double w, double h);
Clipper2Lib::Path64 obround(double cx, double cy, double w, double h);
Clipper2Lib::Path64 regularPolygon(double cx, double cy, double dia, int verts,
                                   double rotDeg = 0.0);

// Append an arc from (x0,y0) to (x1,y1) about (cx,cy) to `out` (in mm, scaled to
// int). The start point is assumed already present in `out`; the end point is
// included. `ccw` selects direction (G03 = ccw, G02 = cw).
void appendArc(Clipper2Lib::Path64 &out, double x0, double y0, double x1, double y1,
               double cx, double cy, bool ccw, double stepDeg = 3.0);

struct Bounds {
    double minX = 0, minY = 0, maxX = 0, maxY = 0;
    bool valid = false;
    void add(double x, double y);
    double width()  const { return maxX - minX; }
    double height() const { return maxY - minY; }
    double cx()     const { return 0.5 * (minX + maxX); }
    double cy()     const { return 0.5 * (minY + maxY); }
};
Bounds bounds(const Clipper2Lib::Paths64 &p);

// Rigid (rotation + translation, no scale) transform. map() applies it in place.
struct Rigid {
    double c = 1, s = 0, tx = 0, ty = 0;
    void map(double &x, double &y) const {
        double nx = c * x - s * y + tx;
        double ny = s * x + c * y + ty;
        x = nx; y = ny;
    }
    double rotationDeg() const;
};

// Least-squares rigid fit of `from` onto `to` (>=2 correspondences).
Rigid solveRigid(const QVector<QPointF> &from, const QVector<QPointF> &to);

} // namespace geo
