#pragma once

// Turns parsed copper geometry / drill holes into G-code:
//  - isolation(): n>=2 V-bit offset passes around the copper (FlatCAM algorithm)
//  - drill(): peck drilling of holes
//  - applyTransform(): rewrite X/Y (and I/J) of an existing program by a rigid
//    transform — used by the camera alignment to register the bottom side.
// Mirroring for double-sided boards is applied at emit time (x' = 2*axisX - x).

#include <QString>
#include <QStringList>
#include <QVector>

#include "clipper2/clipper.h"
#include "Geometry.h"
#include "ExcellonParser.h"

class CamProcessor
{
public:
    struct ToolParams {
        double width = 0.2;        // effective isolation cut width (mm)
        double cutDepth = 0.06;    // milling depth (mm, cuts to -cutDepth)
        double tipAngle = 30.0;    // V-bit included tip angle (deg)
        double tipDia = 0.1;       // V-bit flat tip diameter (mm)
        int    passes = 2;         // isolation passes (>=2)
        double overlap = 0.3;      // fraction of width overlapped per pass (0..0.9)
        double feed = 120.0;       // cutting feed (mm/min)
        double plungeFeed = 60.0;  // plunge feed (mm/min)
        double safeZ = 2.0;        // retract between contours
        double travelZ = 5.0;      // rapid travel height
        double spindleRpm = 0.0;   // >0 emits M3 S<rpm>
        double drillDepth = 1.8;   // drilling depth (mm)
        double drillFeed = 60.0;   // drilling plunge feed (mm/min)
    };

    // Effective V-bit width: params.width if >0, else derived from angle/depth/tip.
    static double effectiveWidth(const ToolParams &p);

    static QStringList isolation(const Clipper2Lib::Paths64 &copper,
                                 const ToolParams &p,
                                 bool mirror, double mirrorAxisX);

    static QStringList drill(const QVector<ExcellonParser::Hole> &holes,
                             const ToolParams &p,
                             bool mirror, double mirrorAxisX);

    static QStringList applyTransform(const QStringList &gcode, const geo::Rigid &t);
};
