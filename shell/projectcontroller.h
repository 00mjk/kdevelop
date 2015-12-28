/* This file is part of KDevelop
Copyright 2006 Adam Treat <treat@kde.org>
Copyright 2007 Andreas Pakulat <apaku@gmx.de>

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

#ifndef KDEVPLATFORM_PROJECTCONTROLLER_H
#define KDEVPLATFORM_PROJECTCONTROLLER_H

#include <interfaces/iprojectcontroller.h>

#include <QUrl>

#include <KIO/UDSEntry>

#include "shellexport.h"

namespace Sublime {
    class Area;
}

namespace KIO {
    class Job;
}

namespace KDevelop
{

class IProject;
class Core;
class Context;
class ContextMenuExtension;
class IPlugin;

class KDEVPLATFORMSHELL_EXPORT IProjectDialogProvider : public QObject
{
Q_OBJECT
public:
    IProjectDialogProvider();
    ~IProjectDialogProvider() override;

public Q_SLOTS:
    /**
     * Displays some UI to ask the user for the project location.
     *
     * @param fetch will tell the UI that the user might want to fetch the project first
     * @param startUrl tells where to look first
     */
    virtual QUrl askProjectConfigLocation(bool fetch, const QUrl& startUrl = QUrl()) = 0;
    virtual bool userWantsReopen() = 0;
};

class KDEVPLATFORMSHELL_EXPORT ProjectController : public IProjectController
{
    Q_OBJECT
    Q_CLASSINFO( "D-Bus Interface", "org.kdevelop.ProjectController" )
    friend class Core;
    friend class CorePrivate;
    friend class ProjectPreferences;

public:
    explicit ProjectController( Core* core );
    ~ProjectController() override;

    IProject* projectAt( int ) const override;
    int projectCount() const override;
    QList<IProject*> projects() const override;

    ProjectBuildSetModel* buildSetModel() override;
    ProjectModel* projectModel() override;
    ProjectChangesModel* changesModel() override;
    virtual QItemSelectionModel* projectSelectionModel();
    IProject* findProjectByName( const QString& name ) override;
    IProject* findProjectForUrl( const QUrl& ) const override;
    void addProject(IProject*);
//     IProject* currentProject() const;

    bool isProjectNameUsed( const QString& name ) const override;
    void setDialogProvider(IProjectDialogProvider*);

    QUrl projectsBaseDirectory() const override;
    QString prettyFileName(const QUrl& url, FormattingOptions format = FormatHtml) const override;
    QString prettyFilePath(const QUrl& url, FormattingOptions format = FormatHtml) const override;

    ContextMenuExtension contextMenuExtension( KDevelop::Context* ctx );

public Q_SLOTS:
    void openProjectForUrl( const QUrl &sourceUrl ) override;
    virtual void fetchProject();
    void openProject( const QUrl &KDev4ProjectFile = QUrl() ) override;
    virtual void abortOpeningProject( IProject* );
    void projectImportingFinished( IProject* );
    void closeProject( IProject* ) override;
    void configureProject( IProject* ) override;

    void reparseProject( IProject* project, bool forceUpdate = false  ) override;

    void eventuallyOpenProjectFile(KIO::Job*,KIO::UDSEntryList);
    void openProjectForUrlSlot(bool);
//     void changeCurrentProject( ProjectBaseItem* );
    void openProjects(const QList<QUrl>& projects);
    void commitCurrentProject();

    // Maps the given path from the source to the equivalent path within the build directory
    // of the corresponding project. If the path is already in the build directory and fallbackRoot is true,
    // then it is mapped to the root of the build directory.
    // If reverse is true, maps the opposite direction, from build to source. [ Used in kdevplatform_shell_environment.sh ]
    Q_SCRIPTABLE QString mapSourceBuild( const QString& path, bool reverse = false, bool fallbackRoot = true ) const;

protected:
    virtual void loadSettings( bool projectIsLoaded );
    virtual void saveSettings( bool projectIsLoaded );

    virtual void initialize();

private:
    //FIXME Do not load all of this just for the project being opened...
    //void legacyLoading();
    void setupActions();
    void cleanup();

    // helper methods for closeProject()
    void unloadUnusedProjectPlugins(IProject* proj);
    void disableProjectCloseAction();
    void closeAllOpenedFiles(IProject* proj);
    void initializePluginCleanup(IProject* proj);

private:
    Q_PRIVATE_SLOT(d, void projectConfig( QObject* ) )
    Q_PRIVATE_SLOT(d, void unloadAllProjectPlugins() )
    Q_PRIVATE_SLOT(d, void updateActionStates( KDevelop::Context* ) )
    Q_PRIVATE_SLOT(d, void closeSelectedProjects() )
    Q_PRIVATE_SLOT(d, void openProjectConfig() )
    Q_PRIVATE_SLOT(d, void areaChanged(Sublime::Area*) )
    class ProjectControllerPrivate* const d;
    friend class ProjectControllerPrivate;
};

class ProjectDialogProvider : public IProjectDialogProvider
{
Q_OBJECT
public:
    explicit ProjectDialogProvider(ProjectControllerPrivate* const p);
    ~ProjectDialogProvider() override;
    ProjectControllerPrivate* const d;

public Q_SLOTS:
    QUrl askProjectConfigLocation(bool fetch, const QUrl& sta) override;
    bool userWantsReopen() override;
};


}
#endif

