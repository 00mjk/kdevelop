/***************************************************************************
 *   Copyright (C) 2007 by Alexander Dymo  <adymo@kdevelop.org>            *
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
#ifndef LANGUAGE_H
#define LANGUAGE_H

#include "ilanguage.h"

#include <QList>

namespace KDevelop {

class KDEVPLATFORMSHELL_EXPORT Language: public ILanguage {
    Q_OBJECT
public:
    explicit Language(ILanguageSupport *support, QObject *parent = 0);
    virtual ~Language();

    virtual void activate();
    virtual void deactivate();

    virtual ILanguageSupport *languageSupport();
    virtual BackgroundParser *backgroundParser();

    static Language *findByName(const QString &name);
    static QList<Language*> findByUrl(const KUrl &url, QObject *parent);


    /**
     * The mutex for the specified \a thread must be held when doing any background parsing.
     */
    virtual QMutex *parseMutex(QThread *thread) const;

    /**
     * Lock all background parser mutexes.
     */
    virtual void lockAllParseMutexes();

    /**
     * Unlock all background parser mutexes.
     */
    virtual void unlockAllParseMutexes();

public Q_SLOTS:
    void threadFinished();

private:
    struct LanguagePrivate *d;
};

}

#endif

//kate: space-indent on; indent-width 4; tab-width: 4; replace-tabs on;
