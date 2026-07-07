#include "ExcellonParser.h"

#include <QRegularExpression>
#include <cmath>

bool ExcellonParser::parse(const QString &text)
{
    m_holes.clear();
    m_toolDia.clear();
    m_warn.clear();
    m_error.clear();
    m_curTool = -1;
    m_header = false;

    static const QRegularExpression toolDefRe(R"(^T(\d+).*C([\d.]+))");
    static const QRegularExpression toolSelRe(R"(^T(\d+)\s*$)");
    static const QRegularExpression coordRe(R"([XY]-?[\d.]+)");

    const QStringList lines = text.split(QRegularExpression("[\r\n]+"),
                                         Qt::SkipEmptyParts);
    for (QString line : lines) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith(';'))
            continue;

        if (line == "M48") { m_header = true; continue; }
        if (line == "%" || line == "M95") { m_header = false; continue; }
        if (line.startsWith("METRIC") || line == "M71") { m_unit = 1.0;  m_decimals = 3; continue; }
        if (line.startsWith("INCH")   || line == "M72") { m_unit = 25.4; m_decimals = 4; continue; }
        if (line.startsWith("M30") || line.startsWith("M00")) break;

        // Tool definition (in header): T1C0.80
        QRegularExpressionMatch md = toolDefRe.match(line);
        if (md.hasMatch()) {
            m_toolDia.insert(md.captured(1).toInt(), md.captured(2).toDouble() * m_unit);
            continue;
        }
        // Tool select (in body): T1
        QRegularExpressionMatch ms = toolSelRe.match(line);
        if (ms.hasMatch()) {
            m_curTool = ms.captured(1).toInt();
            continue;
        }

        // Coordinate line -> a hole at the current tool's diameter.
        if (line.contains('X') || line.contains('Y')) {
            double x = 0, y = 0;
            auto it = coordRe.globalMatch(line);
            while (it.hasNext()) {
                const QString c = it.next().captured(0);
                if (c.startsWith('X')) x = parseCoord(c.mid(1));
                else if (c.startsWith('Y')) y = parseCoord(c.mid(1));
            }
            double dia = m_toolDia.value(m_curTool, 0.0);
            m_holes.push_back({x, y, dia});
        }
    }

    if (m_holes.isEmpty()) {
        m_error = "No holes parsed";
        return false;
    }
    return true;
}

double ExcellonParser::parseCoord(const QString &s) const
{
    if (s.contains('.'))
        return s.toDouble() * m_unit;
    // Fixed-point: assume leading-zero suppression (value = raw / 10^decimals).
    if (!m_warnedFixed) {
        const_cast<ExcellonParser *>(this)->m_warn
            << "Drill file uses fixed-point coordinates; assuming "
               + QString::number(m_decimals) + " decimals (leading-zero suppressed).";
        const_cast<ExcellonParser *>(this)->m_warnedFixed = true;
    }
    bool neg = s.startsWith('-');
    long raw = QString(s).remove('-').toLong();
    double v = (static_cast<double>(raw) / std::pow(10.0, m_decimals)) * m_unit;
    return neg ? -v : v;
}
