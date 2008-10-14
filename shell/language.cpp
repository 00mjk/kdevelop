/***************************************************************************
 *   Copyright 2006 Hamish Rodda <rodda@kde.org>                       *
 *   Copyright 2007 Alexander Dymo <adymo@kdevelop.org>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/
#include "language.h"

#include <QHash>
#include <QMutex>
#include <QThread>

#include <kdebug.h>

#include <language/interfaces/ilanguagesupport.h>

namespace KDevelop {

struct LanguagePrivate {
    ILanguageSupport *support;

    mutable QHash<QThread*, QMutex*> parseMutexes;
    QMutex mutexMutex;
};

Language::Language(ILanguageSupport *support, QObject *parent)
    : ILanguage(support->name(), parent)
{
    kDebug() << "creating language" << support->name();

    d = new LanguagePrivate;
    d->support = support;
}

Language::~Language()
{
    delete d;
}

void Language::deactivate()
{
    kDebug() << "deactivating language" << name();
}

void Language::activate()
{
    kDebug() << "activating language" << name();
}

ILanguageSupport *Language::languageSupport()
{
    return d->support;
}

QMutex *Language::parseMutex(QThread *thread) const
{
    Q_ASSERT(thread);

    QMutexLocker lock(&d->mutexMutex);

    if (!d->parseMutexes.contains(thread)) {
        connect(thread, SIGNAL(finished()), SLOT(threadFinished()));
        d->parseMutexes.insert(thread, new QMutex(QMutex::Recursive));
    }

    return d->parseMutexes[thread];
}

void Language::lockAllParseMutexes()
{
    QMutexLocker lock(&d->mutexMutex);

    QList<QMutex*> waitForLock;

    // Grab the easy pickings first
    QHashIterator<QThread*, QMutex*> it = d->parseMutexes;
    while (it.hasNext()) {
        it.next();
        if (!it.value()->tryLock())
        waitForLock.append(it.value());
    }

    lock.unlock();
    // Work through the stragglers. mutexMutex must be unlocked, else we deadlock.
    foreach (QMutex* mutex, waitForLock)
        mutex->lock();
}

void Language::unlockAllParseMutexes()
{
    QMutexLocker lock(&d->mutexMutex);
    
    foreach(QMutex* mutex, d->parseMutexes)
        mutex->unlock();
}

void Language::threadFinished()
{
    Q_ASSERT(sender());

    QMutexLocker lock(&d->mutexMutex);

    QThread* thread = static_cast<QThread*>(sender());

    Q_ASSERT(d->parseMutexes.contains(thread));

    QMutex* mutex = d->parseMutexes[thread];
    mutex->unlock();
    delete mutex;
    d->parseMutexes.remove(thread);
}

}

#include "language.moc"

