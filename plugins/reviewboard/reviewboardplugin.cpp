/*
 * This file is part of KDevelop
 * Copyright 2010 Aleix Pol Gonzalez <aleixpol@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "reviewboardplugin.h"
#include <QVariantList>

#include <KPluginFactory>
#include <KPluginLoader>
#include <KAboutData>
#include <KLocale>
#include <KDialog>
#include <KMessageBox>
#include <KDebug>
#include <KIO/Job>
#include <QFile>
#include <interfaces/icore.h>
#include <interfaces/ipatchsource.h>
#include <interfaces/iruncontroller.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>
#include <vcs/interfaces/ibasicversioncontrol.h>
#include <vcs/vcsjob.h>
#include "reviewpatchdialog.h"

using namespace KDevelop;

K_PLUGIN_FACTORY(KDevReviewBoardFactory, registerPlugin<ReviewBoardPlugin>(); )
K_EXPORT_PLUGIN(KDevReviewBoardFactory(KAboutData("kdevreviewboard","kdevreviewboard", ki18n("ReviewBoard Support"), "0.1", ki18n("Deal with the ReviewBoard Patches"), KAboutData::License_GPL)))

ReviewBoardPlugin::ReviewBoardPlugin ( QObject* parent, const QVariantList& ) 
    : IPlugin ( KDevReviewBoardFactory::componentData(), parent )
{
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IPatchExporter )
}

ReviewBoardPlugin::~ReviewBoardPlugin()
{}

QByteArray urlToData(const KUrl& url)
{
    QByteArray ret;
    if(url.isLocalFile()) {
        QFile f(url.toLocalFile());
        Q_ASSERT(f.exists());
        bool corr=f.open(QFile::ReadOnly | QFile::Text);
        Q_ASSERT(corr);
        
        ret = f.readAll();
        
    } else {
#warning TODO: add downloading the data
    }
    return ret;
}

void ReviewBoardPlugin::exportPatch(IPatchSource::Ptr source)
{
    IProject* p = ICore::self()->projectController()->findProjectForUrl(source->baseDir());
    KConfigGroup versionedConfig = p->projectConfiguration()->group("ReviewBoard");
    
    ReviewPatchDialog d;
    QString repo;
    if(versionedConfig.hasKey("repository"))
        repo = versionedConfig.readEntry("repository", QString());
    else if(p->versionControlPlugin()) {
        IBasicVersionControl* vcs = p->versionControlPlugin()->extension<IBasicVersionControl>();
        VcsJob* job=vcs->repositoryLocation(p->folder());
        bool ret = job->exec();
        
        if(ret)
            repo = job->fetchResults().toString();
        delete job;
    }
    
    d.setRepository(repo);
    d.setServer(versionedConfig.readEntry("server", KUrl("http://reviewboard.kde.org")));
    d.setUsername(versionedConfig.readEntry("username", QString()));
    d.setPatch(source->file());
    
    int ret = d.exec();
    if(ret==KDialog::Accepted) {
//         postReview(Service(ui.server->text(), ui.username->text(), ui.password->text()), source->file());
    }
}
