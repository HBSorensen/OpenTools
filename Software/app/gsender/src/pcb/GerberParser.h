#pragma once

// Focused RS-274X (Gerber) parser targeting common KiCad copper-layer output.
// Produces a unioned set of copper polygons in Clipper2 integer space (mm*SCALE).
//
// Supported: %FS (leading-zero/abs), %MO mm/in, aperture defs C/R/O/P, D01/D02/D03,
// G01 linear, G02/G03 arcs (G75 multi-quadrant), G36/G37 regions, dark polarity.
// Not supported (warned + skipped): aperture macros %AM, step-repeat %SR, clear
// polarity compositing %LPC.

#include <QString>
#include <QStringList>
#include <QHash>

#include "clipper2/clipper.h"
#include "Geometry.h"

class GerberParser
{
public:
    bool parse(const QString &text);

    // Unioned copper geometry (empty on failure).
    const Clipper2Lib::Paths64 &copper() const { return m_copper; }
    geo::Bounds bounds() const { return geo::bounds(m_copper); }
    const QStringList &warnings() const { return m_warn; }
    QString error() const { return m_error; }

private:
    struct Aperture {
        char type = 'C';                 // C, R, O, P
        double p[4] = {0, 0, 0, 0};      // template parameters (mm)
        int verts = 0;                   // for polygon 'P'
    };

    void handleExtended(const QString &cmd);
    void handleBlock(const QString &block);
    void defineAperture(const QString &body);        // after "ADDnn"
    void flashAperture();                            // D03 at current point
    void flushStroke();                              // emit accumulated open stroke
    Clipper2Lib::Path64 aperturePoly(const Aperture &a, double x, double y) const;
    double coord(long raw) const;                    // raw int -> mm

    // Format / units.
    int m_decimals = 4;
    double m_unit = 1.0;                 // mm per file unit (1.0 mm / 25.4 in)

    // Modal state.
    double m_x = 0, m_y = 0;             // current point (mm)
    int m_interp = 1;                    // 1 linear, 2 cw, 3 ccw
    bool m_region = false;
    int m_curAperture = -1;
    QHash<int, Aperture> m_apertures;

    // Accumulators.
    Clipper2Lib::Path64 m_stroke;        // current open polyline being stroked
    double m_strokeDia = 0;
    Clipper2Lib::Path64 m_regionPath;    // current region contour
    Clipper2Lib::Paths64 m_raw;          // all copper polygons before union
    Clipper2Lib::Paths64 m_copper;       // unioned result

    QStringList m_warn;
    QString m_error;
    bool m_warnedMacro = false;
    bool m_warnedClear = false;
};
