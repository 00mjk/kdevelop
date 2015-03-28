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

#include <QDialog>
#include <QUrl>
#include <QVariantList>

#include <KConfigGroup>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPluginFactory>

#include <interfaces/icore.h>
#include <interfaces/iruncontroller.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>
#include <vcs/interfaces/ibasicversioncontrol.h>
#include <vcs/interfaces/ipatchsource.h>
#include <vcs/vcsjob.h>

#include "reviewpatchdialog.h"
#include "reviewboardjobs.h"
#include "debug.h"

using namespace KDevelop;

typedef QPair<QString, QVariant> VariantPair;
Q_DECLARE_METATYPE(QList<VariantPair>);

K_PLUGIN_FACTORY_WITH_JSON(KDevReviewBoardFactory, "kdevreviewboard.json", registerPlugin<ReviewBoardPlugin>(); )

ReviewBoardPlugin::ReviewBoardPlugin ( QObject* parent, const QVariantList& )
    : IPlugin ( "kdevreviewboard", parent )
{
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IPatchExporter )
}

ReviewBoardPlugin::~ReviewBoardPlugin()
{}

void ReviewBoardPlugin::exportPatch(IPatchSource::Ptr source)
{
    QUrl dirUrl = source->baseDir();
    m_source = source;
    ReviewPatchDialog d(dirUrl);

    IProject* p = ICore::self()->projectController()->findProjectForUrl(dirUrl.adjusted(QUrl::StripTrailingSlash));

    if(p) {
        KConfigGroup versionedConfig = p->projectConfiguration()->group("ReviewBoard");

        if(versionedConfig.hasKey("server")) d.setServer(versionedConfig.readEntry<QUrl>("server", QUrl()));
        if(versionedConfig.hasKey("username")) d.setUsername(versionedConfig.readEntry("username", QString()));
        if(versionedConfig.hasKey("baseDir")) d.setBaseDir(versionedConfig.readEntry("baseDir", "/"));
        if(versionedConfig.hasKey("repository")) d.setRepository(versionedConfig.readEntry("repository", QString()));
    }

    int ret = d.exec();
    if(ret==QDialog::Accepted) {
        KJob* job;
        if (d.isUpdateReview()) {
            job=new ReviewBoard::SubmitPatchRequest(d.server(), source->file(), d.baseDir(), d.review());
            connect(job, &KJob::finished, this, &ReviewBoardPlugin::reviewDone);
        } else {
            job=new ReviewBoard::NewRequest(d.server(), d.repository());
            job->setProperty("extraData", d.extraData());
            connect(job, &KJob::finished, this, &ReviewBoardPlugin::reviewCreated);
        }
        job->setProperty("baseDir", d.baseDir());

        job->start();

        if(p) {
            KConfigGroup versionedConfig = p->projectConfiguration()->group("ReviewBoard");

            // We store username in a diferent field. Unset it from server.
            QUrl storeServer(d.server());
            storeServer.setUserName(QString());
            // Don't store password in plaintext inside .kdev4
            storeServer.setPassword(QString());

            versionedConfig.writeEntry<QUrl>("server", storeServer);
            versionedConfig.writeEntry("username", d.username());
            versionedConfig.writeEntry("baseDir", d.baseDir());
            versionedConfig.writeEntry("repository", d.repository());
        }
    }
}

void ReviewBoardPlugin::reviewDone(KJob* j)
{
    if(j->error()==0) {
        ReviewBoard::SubmitPatchRequest const * job = qobject_cast<ReviewBoard::SubmitPatchRequest*>(j);
        QUrl url = job->server();
        url.setUserInfo(QString());
        QString requrl = QStringLiteral("%1/r/%2/").arg(url.toDisplayString(QUrl::PreferLocalFile)).arg(job->requestId());

        KMessageBox::information(0, i18n("<qt>You can find the new request at:<br /><a href='%1'>%1</a> </qt>", requrl),
                                    QString(), QString(), KMessageBox::AllowLink);
    } else {
        KMessageBox::error(0, j->errorText());
    }
}

void ReviewBoardPlugin::reviewCreated(KJob* j)
{
    if (j->error()==0) {
        ReviewBoard::NewRequest const * job = qobject_cast<ReviewBoard::NewRequest*>(j);

        //This will provide things like groups and users for review from .reviewboardrc
        QVariantMap extraData = job->property("extraData").toMap();
        if (!extraData.isEmpty()) {
            KJob* updateJob = new ReviewBoard::UpdateRequest(job->server(), job->requestId(), extraData);
            updateJob->start();
        }

        // for git projects, m_source will be a VCSDiffPatchSource instance
        ReviewBoard::SubmitPatchRequest* submitPatchJob=new ReviewBoard::SubmitPatchRequest(job->server(), m_source->file(), j->property("baseDir").toString(), job->requestId());
        connect(submitPatchJob, &ReviewBoard::SubmitPatchRequest::finished, this, &ReviewBoardPlugin::reviewDone);
        submitPatchJob->start();
    } else {
        KMessageBox::error(0, j->errorText());
    }
}

#include "reviewboardplugin.moc"
