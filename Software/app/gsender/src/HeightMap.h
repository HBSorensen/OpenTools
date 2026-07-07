#pragma once

#include <QVector>
#include <QStringList>
#include <QString>

// A rectangular grid of probed surface heights (work coordinates, mm) with
// bilinear interpolation. Heights are normalized so the grid origin (col 0,
// row 0) reads 0 — i.e. Z zero is taken at the origin and the map expresses the
// surface deviation everywhere else.
class HeightMap
{
public:
    void configure(double x0, double y0, double width, double height,
                   int cols, int rows);
    void clear();

    bool valid() const { return m_valid; }
    int cols() const { return m_cols; }
    int rows() const { return m_rows; }
    double x0() const { return m_x0; }
    double y0() const { return m_y0; }
    double dx() const { return m_dx; }
    double dy() const { return m_dy; }

    // Grid point coordinates.
    double pointX(int col) const { return m_x0 + col * m_dx; }
    double pointY(int row) const { return m_y0 + row * m_dy; }

    void setZ(int col, int row, double z);        // store raw probed Z
    double at(int col, int row) const;            // normalized (origin = 0)

    // Call once all points are probed: fixes the reference (origin) and the
    // normalized min/max, and marks the map valid for interpolation.
    void finalize();

    double minZ() const { return m_min; }         // normalized
    double maxZ() const { return m_max; }         // normalized

    // Interpolated, normalized height offset at an arbitrary XY (clamped to the
    // grid bounds). Returns 0 if the map is not valid.
    double interp(double x, double y) const;

private:
    int index(int col, int row) const { return row * m_cols + col; }

    bool m_valid = false;
    int m_cols = 0, m_rows = 0;
    double m_x0 = 0, m_y0 = 0;
    double m_dx = 0, m_dy = 0;
    double m_ref = 0;                 // raw Z at the origin (subtracted out)
    double m_min = 0, m_max = 0;      // normalized
    QVector<double> m_z;             // raw probed Z, size cols*rows
};

// Rewrite a G-code program so cutting moves follow the probed surface.
//
// Feed moves (G1) are subdivided in XY to at most `segLen` mm and each point's
// Z is offset by the interpolated map height; plunge (Z-only G1) endpoints are
// offset; rapids (G0) pass through untouched. Arcs (G2/G3) get an endpoint Z
// offset with a one-time warning.
//
// If the program uses relative positioning (G91) or inch units (G20), the
// transform is unsafe and is skipped: the original program is returned and a
// message is written to `warning`.
QStringList applyHeightMap(const QStringList &src, const HeightMap &map,
                           double segLen, QString &warning);
