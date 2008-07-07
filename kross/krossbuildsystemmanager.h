/* KDevPlatform Kross Support
 *
 * Copyright 2008 Aleix Pol <aleixpol@gmail.com>
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

#ifndef IKROSSPROJECTMANAGER_H
#define IKROSSPROJECTMANAGER_H

#include <QObject>
#include <QList>

#include <iprojectfilemanager.h>
#include <ibuildsystemmanager.h>
#include <iplugin.h>

namespace Kross { class Action; }

class KrossBuildSystemManager : public KDevelop::IBuildSystemManager
{
public:
    explicit KrossBuildSystemManager(const QVariantList& o=QVariantList());

    void setAction(Kross::Action* anAction);

    QList<KDevelop::ProjectFolderItem*> parse( KDevelop::ProjectFolderItem* dom );
    KDevelop::ProjectFolderItem* import(KDevelop::IProject *project );

    KDevelop::IProjectBuilder* builder(KDevelop::ProjectFolderItem*) const;
    KUrl buildDirectory(KDevelop::ProjectBaseItem*) const;

    KUrl::List includeDirectories(KDevelop::ProjectBaseItem *) const;
    QHash<QString,QString> defines(KDevelop::ProjectBaseItem *) const;
    QList<KDevelop::ProjectTargetItem*> targets() const;
    QList<KDevelop::ProjectTargetItem*> targets(KDevelop::ProjectFolderItem*) const;

    KDevelop::ProjectFolderItem* addFolder( const KUrl& /*folder */, KDevelop::ProjectFolderItem* /*parent*/ );
    KDevelop::ProjectTargetItem* createTarget( const QString&, KDevelop::ProjectFolderItem* );
    KDevelop::ProjectFileItem* addFile( const KUrl&, KDevelop::ProjectFolderItem* );
    bool addFileToTarget( KDevelop::ProjectFileItem*, KDevelop::ProjectTargetItem* );
    bool removeFolder( KDevelop::ProjectFolderItem* );
    bool removeTarget( KDevelop::ProjectTargetItem* );
    bool removeFile( KDevelop::ProjectFileItem* );
    bool removeFileFromTarget( KDevelop::ProjectFileItem*, KDevelop::ProjectTargetItem* );
    bool renameFile(KDevelop::ProjectFileItem*, const KUrl&);
    bool renameFolder(KDevelop::ProjectFolderItem*, const KUrl&);
    QHash<QString,QString> environment(KDevelop::ProjectBaseItem *) const;
    Features features() const;

    void addFile(const QString& folder, const QString& targetName, const QString& filename);
    void addTarget(const QString& folder, const QString& targetName);
    void addFolder(const QString& folder);
    /*
signals:
    void init(const QVariantList& args);
    void parse( const KUrl& folder );
    void importProject(KDevelop::IProject* project);

    void createFolder(const KUrl& folder);
    void createFile(const KUrl& file);
    void createTarget(const KUrl& folder, const QString& targetname);
    void addFileToTarget(const KUrl& folder, const QString& targetname, const KUrl& filename);
    void rename(const KUrl& from, const KUrl& to); //File or folder
    void remove(const KUrl& file); //File or folder
    void removeTarget(const KUrl& folder, const QString& targetname);
    void removeFileFromTarget(const KUrl& folder, const QString& targetname, const KUrl& filename);*/

private:
    Kross::Action *action;
    KDevelop::IProjectFileManager::Features m_features;
    QMap<KDevelop::ProjectFolderItem*, QList<KDevelop::ProjectTargetItem*> > m_targets;
    QMap<KUrl, KDevelop::ProjectFolderItem*> m_folderPerUrl;
    QMap<QString, KDevelop::ProjectTargetItem*> m_targetPerName;
};


#endif
