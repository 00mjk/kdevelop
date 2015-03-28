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

#include "pastebinplugin.h"

#include <QFile>
#include <QVariantList>

#include <vcs/interfaces/ipatchsource.h>

#include <KIO/TransferJob>
#include <KJobTrackerInterface>
#include <KLocalizedString>
#include <KMessageBox>
#include <KPluginFactory>

using namespace KDevelop;

Q_LOGGING_CATEGORY(PLUGIN_PASTEBIN, "kdevplatform.plugins.pastebin")
K_PLUGIN_FACTORY_WITH_JSON(KDevPastebinFactory, "kdevpastebin.json", registerPlugin<PastebinPlugin>();)

PastebinPlugin::PastebinPlugin ( QObject* parent, const QVariantList& )
    : IPlugin ( "kdevpastebin", parent )
{
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IPatchExporter )
}

PastebinPlugin::~PastebinPlugin()
{}

namespace
{
QByteArray urlToData(const QUrl& url)
{
    QByteArray ret;
    if(url.isLocalFile()) {
        QFile f(url.toLocalFile());
        Q_ASSERT(f.exists());
        bool corr=f.open(QFile::ReadOnly | QFile::Text);
        Q_ASSERT(corr);
        Q_UNUSED(corr);

        ret = f.readAll();

    } else {
//TODO: add downloading the data
    }
    return ret;
}
}

void PastebinPlugin::exportPatch(IPatchSource::Ptr source)
{
    qCDebug(PLUGIN_PASTEBIN) << "exporting patch to pastebin" << source->file();
    QByteArray bytearray = "api_option=paste&api_paste_private=1&api_paste_name=kdevelop-pastebin-plugin&api_paste_expire_date=1D&api_paste_format=diff&api_dev_key=0c8b6add8e0f6d53f61fe5ce870a1afa&api_paste_code="+QUrl::toPercentEncoding(urlToData(source->file()), "/");

    QUrl url("http://pastebin.com/api/api_post.php");

    KIO::TransferJob *tf = KIO::http_post(url, bytearray);

    tf->addMetaData("content-type","Content-Type: application/x-www-form-urlencoded");
    connect(tf, &KIO::TransferJob::data, this, &PastebinPlugin::data);

    m_result.insert(tf, QByteArray());
    KIO::getJobTracker()->registerJob(tf);
}

void PastebinPlugin::data(KIO::Job* job, const QByteArray &data)
{
    QMap< KIO::Job*, QString >::iterator it = m_result.find(job);
    Q_ASSERT(it!=m_result.end());
    if (data.isEmpty()) {
        if (job->error()) {
            KMessageBox::error(0, job->errorString());
        } else if (it->isEmpty() || it->startsWith("ERROR")) {
            KMessageBox::error(0, *it);
        } else {
            QString htmlLink=QStringLiteral("<a href='%1'>%1</a>").arg(*it);
            KMessageBox::information(0, i18nc("The parameter is the link where the patch is stored", "<qt>You can find your patch at:<br/>%1</qt>", htmlLink), QString(), QString(), KMessageBox::AllowLink | KMessageBox::Notify);
        }
        m_result.erase(it);
    } else {
        *it += data;
    }
}

#include "pastebinplugin.moc"
