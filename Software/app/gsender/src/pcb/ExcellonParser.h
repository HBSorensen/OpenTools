#pragma once

// Minimal Excellon drill-file parser (KiCad output). Extracts hole positions and
// diameters. Handles METRIC/INCH, tool defs (T#C#), tool select, and coordinates
// in either explicit-decimal or fixed-point form. Used for board drilling and for
// the 3-hole alignment file.

#include <QString>
#include <QStringList>
#include <QVector>
#include <QHash>

class ExcellonParser
{
public:
    struct Hole { double x = 0, y = 0, dia = 0; };

    bool parse(const QString &text);
    const QVector<Hole> &holes() const { return m_holes; }
    const QStringList &warnings() const { return m_warn; }
    QString error() const { return m_error; }

private:
    double parseCoord(const QString &s) const;

    double m_unit = 1.0;      // mm per file unit
    int m_decimals = 3;       // fixed-point decimals fallback
    bool m_header = false;
    int m_curTool = -1;
    QHash<int, double> m_toolDia;
    QVector<Hole> m_holes;
    QStringList m_warn;
    QString m_error;
    bool m_warnedFixed = false;
};
