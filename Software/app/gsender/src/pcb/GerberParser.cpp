#include "GerberParser.h"

#include <QRegularExpression>
#include <cmath>
#include <utility>

using namespace Clipper2Lib;

bool GerberParser::parse(const QString &text)
{
    m_raw.clear();
    m_copper.clear();
    m_warn.clear();
    m_error.clear();
    m_apertures.clear();
    m_x = m_y = 0;
    m_interp = 1;
    m_region = false;
    m_curAperture = -1;
    m_stroke.clear();
    m_regionPath.clear();

    const QString t = text;
    int i = 0, n = t.size();
    while (i < n) {
        QChar ch = t[i];
        if (ch == '%') {
            int end = t.indexOf('%', i + 1);
            if (end < 0) { m_error = "Unterminated % block"; return false; }
            QString ext = t.mid(i + 1, end - i - 1);
            // An extended block may hold several '*'-separated sub-commands.
            for (const QString &sub : ext.split('*', Qt::SkipEmptyParts))
                handleExtended(sub.trimmed());
            i = end + 1;
        } else if (ch == '*') {
            i++;
        } else {
            int end = t.indexOf('*', i);
            if (end < 0) end = n;
            handleBlock(t.mid(i, end - i).trimmed());
            i = (end < n) ? end + 1 : n;
        }
    }

    flushStroke();
    if (m_raw.empty()) {
        m_error = "No copper geometry parsed";
        return false;
    }
    m_copper = Union(m_raw, FillRule::NonZero);
    return true;
}

double GerberParser::coord(long raw) const
{
    return (static_cast<double>(raw) / std::pow(10.0, m_decimals)) * m_unit;
}

void GerberParser::handleExtended(const QString &cmd)
{
    if (cmd.size() < 2) return;
    const QString code = cmd.left(2);

    if (code == "FS") {
        // e.g. LAX34Y34 -> decimals = second digit after X
        int xi = cmd.indexOf('X');
        if (xi >= 0 && xi + 2 < cmd.size())
            m_decimals = cmd.mid(xi + 2, 1).toInt();
    } else if (code == "MO") {
        m_unit = cmd.contains("IN") ? 25.4 : 1.0;
    } else if (code == "AD") {
        defineAperture(cmd.mid(2));            // "D10C,0.5"
    } else if (code == "LP") {
        if (cmd.contains('C') && !m_warnedClear) {
            m_warn << "Clear polarity (%LPC) not supported — treated as dark.";
            m_warnedClear = true;
        }
    } else if (code == "AM") {
        if (!m_warnedMacro) {
            m_warn << "Aperture macros (%AM) not supported — those flashes skipped.";
            m_warnedMacro = true;
        }
    } else if (code == "SR") {
        m_warn << "Step-repeat (%SR) not supported — ignored.";
    }
}

void GerberParser::defineAperture(const QString &body)
{
    // body: D<code><type>,<p1>X<p2>...   e.g. "D10C,0.5" or "D11R,1X0.5"
    static const QRegularExpression re(R"(^D(\d+)([A-Z])(?:,(.*))?$)");
    const QRegularExpressionMatch m = re.match(body);
    if (!m.hasMatch()) {
        // Custom (macro) aperture reference — cannot render without the macro.
        return;
    }
    int code = m.captured(1).toInt();
    Aperture a;
    a.type = m.captured(2).at(0).toLatin1();
    const QStringList params = m.captured(3).split('X', Qt::SkipEmptyParts);
    for (int k = 0; k < params.size() && k < 4; ++k)
        a.p[k] = params[k].toDouble() * m_unit;
    if (a.type == 'P' && params.size() >= 2)
        a.verts = int(params[1].toDouble());   // vertices are a count, not mm
    m_apertures.insert(code, a);
}

Path64 GerberParser::aperturePoly(const Aperture &a, double x, double y) const
{
    switch (a.type) {
    case 'R': return geo::rectCentered(x, y, a.p[0], a.p[1]);
    case 'O': return geo::obround(x, y, a.p[0], a.p[1]);
    case 'P': return geo::regularPolygon(x, y, a.p[0], a.verts > 0 ? a.verts : 6,
                                         a.p[2]);
    case 'C':
    default:  return geo::circle(x, y, a.p[0] / 2.0);
    }
}

void GerberParser::flashAperture()
{
    auto it = m_apertures.find(m_curAperture);
    if (it == m_apertures.end())
        return;                                // unknown/macro aperture: skip
    m_raw.push_back(aperturePoly(*it, m_x, m_y));
}

void GerberParser::flushStroke()
{
    if (m_stroke.size() >= 2 && m_strokeDia > 0) {
        Paths64 in{m_stroke};
        Paths64 out = InflatePaths(in, m_strokeDia / 2.0 * geo::SCALE,
                                   JoinType::Round, EndType::Round);
        for (auto &p : out)
            m_raw.push_back(std::move(p));
    } else if (m_stroke.size() == 1 && m_strokeDia > 0) {
        m_raw.push_back(geo::circle(geo::MM(m_stroke[0].x), geo::MM(m_stroke[0].y),
                                    m_strokeDia / 2.0));
    }
    m_stroke.clear();
}

void GerberParser::handleBlock(const QString &block)
{
    if (block.isEmpty() || block.startsWith("G04"))
        return;

    static const QRegularExpression tok(R"(([A-Za-z])([+-]?\d+))");
    auto it = tok.globalMatch(block);

    bool haveX = false, haveY = false, haveI = false, haveJ = false;
    double nx = m_x, ny = m_y, iOff = 0, jOff = 0;
    int op = 0;   // 1/2/3 = D01/02/03

    while (it.hasNext()) {
        const QRegularExpressionMatch m = it.next();
        const char letter = m.captured(1).at(0).toUpper().toLatin1();
        const long val = m.captured(2).toLong();
        switch (letter) {
        case 'G':
            switch (val) {
            case 1: m_interp = 1; break;
            case 2: m_interp = 2; break;
            case 3: m_interp = 3; break;
            case 36: flushStroke(); m_region = true; m_regionPath.clear(); break;
            case 37:
                if (m_regionPath.size() >= 3)
                    m_raw.push_back(m_regionPath);
                m_regionPath.clear();
                m_region = false;
                break;
            case 74:
                m_warn << "Single-quadrant arcs (G74) not supported; assuming G75.";
                break;
            default: break;   // G75, G54, G01-combos handled above
            }
            break;
        case 'X': haveX = true; nx = coord(val); break;
        case 'Y': haveY = true; ny = coord(val); break;
        case 'I': haveI = true; iOff = coord(val); break;
        case 'J': haveJ = true; jOff = coord(val); break;
        case 'D':
            if (val == 1 || val == 2 || val == 3) {
                op = int(val);
            } else if (val >= 10) {
                flushStroke();
                m_curAperture = int(val);
                auto ap = m_apertures.find(m_curAperture);
                m_strokeDia = (ap != m_apertures.end()) ? ap->p[0] : 0.0;
            }
            break;
        case 'M': break;      // M02 end etc.
        default: break;
        }
    }
    (void)haveI; (void)haveJ;

    if (op == 0)
        return;

    const bool ccw = (m_interp == 3);
    const double cx = m_x + iOff, cy = m_y + jOff;

    if (op == 2) {                     // D02 move
        flushStroke();
        if (m_region) {
            if (m_regionPath.size() >= 3)
                m_raw.push_back(m_regionPath);
            m_regionPath.clear();
            m_regionPath.push_back(Point64(geo::I(nx), geo::I(ny)));
        }
        m_x = nx; m_y = ny;
    } else if (op == 1) {              // D01 draw / interpolate
        if (m_region) {
            if (m_regionPath.empty())
                m_regionPath.push_back(Point64(geo::I(m_x), geo::I(m_y)));
            if (m_interp == 1)
                m_regionPath.push_back(Point64(geo::I(nx), geo::I(ny)));
            else
                geo::appendArc(m_regionPath, m_x, m_y, nx, ny, cx, cy, ccw);
        } else {
            if (m_stroke.empty())
                m_stroke.push_back(Point64(geo::I(m_x), geo::I(m_y)));
            if (m_interp == 1)
                m_stroke.push_back(Point64(geo::I(nx), geo::I(ny)));
            else
                geo::appendArc(m_stroke, m_x, m_y, nx, ny, cx, cy, ccw);
        }
        m_x = nx; m_y = ny;
    } else if (op == 3) {              // D03 flash
        flushStroke();
        m_x = nx; m_y = ny;
        flashAperture();
    }
}
