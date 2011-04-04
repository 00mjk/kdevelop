/* KDevelop QMake Support
 *
 * Copyright 2006 Andreas Pakulat <apaku@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef QMAKEPROJECTFILE_H
#define QMAKEPROJECTFILE_H

#include "qmakefile.h"

class QMakeMkSpecs;

template <typename T> class QList;

class QMakeMkSpecs;
class QMakeCache;

namespace KDevelop {
class IProject;
}

class QMakeProjectFile : public QMakeFile
{
public:
    typedef QPair< QString, QString > DefinePair;
    static const QStringList FileVariables;

    QMakeProjectFile( const QString& projectfile );
    ~QMakeProjectFile();

    bool read();

    QStringList subProjects() const;
    bool hasSubProject(const QString& file) const;

    KUrl::List files() const;
    KUrl::List filesForTarget( const QString& ) const;
    KUrl::List includeDirectories() const;

    QStringList targets() const;

    QString getTemplate() const;

    void setMkSpecs( QMakeMkSpecs* mkspecs );
    QMakeMkSpecs* mkSpecs() const;
    void setQMakeCache( QMakeCache* cache );
    QMakeCache* qmakeCache() const;
    QStringList resolveVariables( const QString& variable ) const;
    QList< DefinePair > defines() const;

    /// required for proper build-dir resolution
    void setProject(KDevelop::IProject* project);
    KDevelop::IProject* project() const;

    KUrl buildDir() const;

private:
    void addUrlsForVariable(const QString& variable, KUrl::List* list) const;

    QMakeMkSpecs* m_mkspecs;
    QMakeCache* m_cache;
    KDevelop::IProject* m_project;
    static QString m_qtIncludeDir;
};

#endif

