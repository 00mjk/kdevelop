/*
   Copyright 2008 David Nolden <david.nolden.kdevelop@art-master.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
 */

#include "modificationrevision.h"

#include <QString>
#include <QFileInfo>

#include <serialization/indexedstring.h>
#include "modificationrevisionset.h"

#include <KTextEditor/Document>

/// @todo Listen to filesystem changes (together with the project manager)
/// and call fileModificationCache().clear(...) when a file has changed

using namespace KDevelop;

const int KDevelop::cacheModificationTimesForSeconds = 30;

QMutex fileModificationTimeCacheMutex(QMutex::Recursive);

struct FileModificationCache
{
    QDateTime m_readTime;
    QDateTime m_modificationTime;
};
Q_DECLARE_TYPEINFO(FileModificationCache, Q_MOVABLE_TYPE);

using FileModificationMap = QHash<KDevelop::IndexedString, FileModificationCache>;

FileModificationMap& fileModificationCache()
{
    static FileModificationMap cache;
    return cache;
}

using OpenDocumentRevisionsMap = QHash<KDevelop::IndexedString, int>;

OpenDocumentRevisionsMap& openDocumentsRevisionMap()
{
    static OpenDocumentRevisionsMap map;
    return map;
}

QDateTime fileModificationTimeCached(const IndexedString& fileName)
{
    const auto currentTime = QDateTime::currentDateTime();

    auto it = fileModificationCache().constFind(fileName);
    if (it != fileModificationCache().constEnd()) {
        ///Use the cache for X seconds
        if (it.value().m_readTime.secsTo(currentTime) < cacheModificationTimesForSeconds) {
            return it.value().m_modificationTime;
        }
    }

    QFileInfo fileInfo(fileName.str());
    FileModificationCache data = {currentTime, fileInfo.lastModified()};
    fileModificationCache().insert(fileName, data);
    return data.m_modificationTime;
}

void ModificationRevision::clearModificationCache(const IndexedString& fileName)
{
    ///@todo Make the cache management more clever (don't clear the whole)
    ModificationRevisionSet::clearCache();

    QMutexLocker lock(&fileModificationTimeCacheMutex);

    fileModificationCache().remove(fileName);
}

ModificationRevision ModificationRevision::revisionForFile(const IndexedString& url)
{
    QMutexLocker lock(&fileModificationTimeCacheMutex);

    ModificationRevision ret(fileModificationTimeCached(url));

    OpenDocumentRevisionsMap::const_iterator it = openDocumentsRevisionMap().constFind(url);
    if (it != openDocumentsRevisionMap().constEnd()) {
        ret.revision = it.value();
    }

    return ret;
}

void ModificationRevision::clearEditorRevisionForFile(const KDevelop::IndexedString& url)
{
    ModificationRevisionSet::clearCache(); ///@todo Make the cache management more clever (don't clear the whole)

    QMutexLocker lock(&fileModificationTimeCacheMutex);
    openDocumentsRevisionMap().remove(url);
}

void ModificationRevision::setEditorRevisionForFile(const KDevelop::IndexedString& url, int revision)
{
    ModificationRevisionSet::clearCache(); ///@todo Make the cache management more clever (don't clear the whole)

    QMutexLocker lock(&fileModificationTimeCacheMutex);
    openDocumentsRevisionMap().insert(url, revision);
    Q_ASSERT(revisionForFile(url).revision == revision);
}

ModificationRevision::ModificationRevision(const QDateTime& modTime, int revision_)
    : modificationTime(modTime.toSecsSinceEpoch())
    , revision(revision_)
{
}

bool ModificationRevision::operator <(const ModificationRevision& rhs) const
{
    return modificationTime < rhs.modificationTime ||
           (modificationTime == rhs.modificationTime && revision < rhs.revision);
}

bool ModificationRevision::operator ==(const ModificationRevision& rhs) const
{
    return modificationTime == rhs.modificationTime && revision == rhs.revision;
}

bool ModificationRevision::operator !=(const ModificationRevision& rhs) const
{
    return modificationTime != rhs.modificationTime || revision != rhs.revision;
}

QString ModificationRevision::toString() const
{
    return QStringLiteral("%1 (rev %2)").arg(QDateTime::fromSecsSinceEpoch(modificationTime, Qt::LocalTime).time().toString()).arg(revision);
}
