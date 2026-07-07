#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QString>

// Lists G-code programs (*.nc/*.gcode/*.ngc/*.tap) found on the writable data
// partition (GSENDER_DATA, default /data) and on any auto-mounted USB sticks
// (under /run/media). Call refresh() to rescan.
class NcFileModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        PathRole,
        SizeRole,
        SourceRole   // "data" or "usb"
    };

    explicit NcFileModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void refresh();

signals:
    void countChanged();

private:
    struct Entry {
        QString name;
        QString path;
        qint64 size;
        QString source;
    };
    void scanDir(const QString &dir, const QString &source);
    QVector<Entry> m_entries;
};
