#include "NcFileModel.h"

#include <QDir>
#include <QFileInfo>
#include <QProcessEnvironment>

NcFileModel::NcFileModel(QObject *parent) : QAbstractListModel(parent)
{
    refresh();
}

int NcFileModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant NcFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const Entry &e = m_entries.at(index.row());
    switch (role) {
    case NameRole:   return e.name;
    case PathRole:   return e.path;
    case SizeRole:   return e.size;
    case SourceRole: return e.source;
    default:         return {};
    }
}

QHash<int, QByteArray> NcFileModel::roleNames() const
{
    return {
        {NameRole,   "name"},
        {PathRole,   "path"},
        {SizeRole,   "size"},
        {SourceRole, "source"},
    };
}

void NcFileModel::scanDir(const QString &dir, const QString &source)
{
    QDir d(dir);
    if (!d.exists())
        return;
    const QStringList filters{"*.nc", "*.NC", "*.gcode", "*.ngc", "*.gc", "*.tap"};
    const QFileInfoList files = d.entryInfoList(filters, QDir::Files, QDir::Name);
    for (const QFileInfo &fi : files)
        m_entries.push_back({fi.fileName(), fi.absoluteFilePath(), fi.size(), source});
}

void NcFileModel::refresh()
{
    beginResetModel();
    m_entries.clear();

    // Writable data partition (settings + saved programs).
    const QString dataDir =
        QProcessEnvironment::systemEnvironment().value("GSENDER_DATA", "/data");
    scanDir(dataDir, QStringLiteral("data"));

    // Every auto-mounted USB stick under /run/media/<dev>.
    QDir media("/run/media");
    if (media.exists()) {
        const QStringList mounts = media.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
        for (const QString &m : mounts)
            scanDir(media.absoluteFilePath(m), QStringLiteral("usb"));
    }

    endResetModel();
    emit countChanged();
}
