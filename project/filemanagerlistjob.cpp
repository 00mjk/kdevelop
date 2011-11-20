/* This file is part of KDevelop
    Copyright 2009  Radu Benea <radub82@gmail.com>

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public License
    along with this library; see the file COPYING.LIB.  If not, write to
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "filemanagerlistjob.h"

#include <interfaces/iproject.h>
#include <project/projectmodel.h>

#include <KDebug>

using namespace KDevelop;

FileManagerListJob::FileManagerListJob(ProjectFolderItem* item, const bool forceRecursion)
    : KIO::Job(), m_item(0), m_forceRecursion(forceRecursion), m_aborted(false)
{
    /* the following line is not an error in judgement, apparently starting a
     * listJob while the previous one hasn't self-destructed takes a lot of time,
     * so we give the job a chance to selfdestruct first */
    connect( this, SIGNAL(nextJob()), SLOT(startNextJob()), Qt::QueuedConnection );

    addSubDir(item);
    startNextJob();
}

ProjectFolderItem* FileManagerListJob::item() const
{
    return m_item;
}

void FileManagerListJob::addSubDir( ProjectFolderItem* item )
{
    Q_ASSERT(!m_item || item->url().upUrl() == m_item->url());

    m_listQueue.enqueue(item);
}

void FileManagerListJob::removeSubDir(ProjectFolderItem* item)
{
    m_listQueue.removeAll(item);
}

void FileManagerListJob::slotEntries(KIO::Job* job, const KIO::UDSEntryList& entriesIn)
{
    Q_UNUSED(job);
    entryList.append(entriesIn);
}

void FileManagerListJob::startNextJob()
{
    if ( m_listQueue.isEmpty() || m_aborted ) {
        return;
    }

    m_item = m_listQueue.dequeue();
    KIO::ListJob* job = KIO::listDir( m_item->url(), KIO::HideProgressInfo );
    job->setParentJob( this );
    connect( job, SIGNAL(entries(KIO::Job*,KIO::UDSEntryList)),
             this, SLOT(slotEntries(KIO::Job*,KIO::UDSEntryList)) );
    connect( job, SIGNAL(result(KJob*)), SLOT(slotResult(KJob*)) );
}

void FileManagerListJob::slotResult(KJob* job)
{
    if (m_aborted) {
        return;
    }

    emit entries(this, m_item, entryList, m_forceRecursion);
    entryList.clear();

    if( job->error() ) {
        kDebug(9517) << "error in list job:" << job->error() << job->errorString();
    }

    if( m_listQueue.isEmpty() ) {
        emitResult();
    } else {
        emit nextJob();
    }
}

void FileManagerListJob::abort()
{
    bool killed = kill();
    Q_ASSERT(killed);
    Q_UNUSED(killed);

    m_aborted = true;
}

#include "filemanagerlistjob.moc"
