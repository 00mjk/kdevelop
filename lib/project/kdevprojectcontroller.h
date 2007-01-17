/* This file is part of KDevelop
Copyright (C) 2006 Adam Treat <treat@kde.org>

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

#ifndef KDEVPROJECTCONTROLLER_H
#define KDEVPROJECTCONTROLLER_H

#include <QObject>
#include "kdevcore.h"

#include "kdevexport.h"

#include <kurl.h>

class KRecentFilesAction;

namespace Koncrete
{

class Plugin;
class Project;


class KDEVPLATFORM_EXPORT ProjectController : public QObject, protected CoreInterface
{
    friend class Core;
    Q_OBJECT
public:
    ProjectController( QObject *parent = 0 );
    virtual ~ProjectController();

    KUrl localFile() const;
    void setLocalFile( const KUrl &localFile );

    KUrl globalFile() const;
    void setGlobalFile( const KUrl &globalFile );

    KUrl projectsDirectory() const;
    void setProjectsDirectory( const KUrl &projectsDir );

    bool isLoaded() const;
    KUrl projectDirectory() const;

    Project* activeProject() const;

public Q_SLOTS:
    bool openProject( const KUrl &KDev4ProjectFile = KUrl() );
    bool closeProject();

Q_SIGNALS:
    void projectOpened();
    void projectClosing();
    void projectClosed();

protected:
    virtual void loadSettings( bool projectIsLoaded );
    virtual void saveSettings( bool projectIsLoaded );
    virtual void initialize();
    virtual void cleanup();

private:
    //FIXME Do not load all of this just for the project being opened...
    //void legacyLoading();
    bool loadProjectPart();

private:
    QString m_name;
    KUrl m_localFile;
    KUrl m_globalFile;
    KUrl m_projectsDir;
    KUrl m_lastProject;
    bool m_isLoaded;
    Project* m_project;
    Plugin* m_projectPart;
    KRecentFilesAction *m_recentAction;
};

}
#endif

// kate: space-indent on; indent-width 4; tab-width 4; replace-tabs on
