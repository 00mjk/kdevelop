/***************************************************************************
 *   Copyright 2007 Robert Gruber <rgruber@users.sourceforge.net>          *
 *   Copyright 2007 Dukju Ahn <dukjuahn@gmail.com>                         *
 *                                                                         *
 *   Adapted for Git                                                       *
 *   Copyright 2008 Evgeniy Ivanov <powerfox@kde.ru>                       *
 *                                                                         *
 *   Adapted for Hg                                                        *
 *   Copyright 2008 Thomas Burdick <thomas.burdick@gmail.com>              *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or         *
 *   modify it under the terms of the GNU General Public License as        *
 *   published by the Free Software Foundation; either version 2 of        *
 *   the License or (at your option) version 3 or any later version        *
 *   accepted by the membership of KDE e.V. (or its successor approved     *
 *   by the membership of KDE e.V.), which shall act as a proxy            *
 *   defined in Section 14 of version 3 of the license.                    *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#include "hgplugin.h"

#include <QtGui/QAction>
#include <QtGui/QMenu>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>

#include <KParts/PartManager>
#include <KParts/Part>
#include <KPluginFactory>
#include <KPluginLoader>
#include <KActionCollection>
#include <KMessageBox>
#include <KUrl>
#include <KAction>


#include <interfaces/iproject.h>
#include <projectmodel.h>
#include <iuicontroller.h>
#include <icore.h>
#include <context.h>
#include <vcsmapping.h>

#include<interfaces/contextmenuextension.h>

#include "hgproxy.h"
#include "hgjob.h"
#include "commitdialog.h"
#include "importdialog.h"
#include "hgmainview.h"
#include "importmetadatawidget.h"
// #include "diffoptionsdialog.h"
// #include "editorsview.h"
// #include "logview.h"
// #include "annotateview.h"

K_PLUGIN_FACTORY(KDevHgFactory, registerPlugin<HgPlugin>(); )
K_EXPORT_PLUGIN(KDevHgFactory("kdevhg"))

class KDevHgViewFactory: public KDevelop::IToolViewFactory{
public:
    KDevHgViewFactory(HgPlugin *plugin): m_plugin(plugin) {}
    virtual QWidget* create(QWidget *parent = 0)
    {
        return new HgMainView(m_plugin, parent);
    }
    virtual Qt::DockWidgetArea defaultPosition()
    {
    return Qt::BottomDockWidgetArea;
    }
    virtual QString id() const
    {
        return "org.kdevelop.HgView";
    }

private:
    HgPlugin *m_plugin;
};

class HgPluginPrivate {
public:
    KDevHgViewFactory* m_factory;
    QPointer<HgProxy> m_proxy;
    KUrl::List m_ctxUrlList;
};

HgPlugin::HgPlugin( QObject *parent, const QVariantList & )
    : KDevelop::IPlugin(KDevHgFactory::componentData(), parent)
{
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IBasicVersionControl )
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IDistributedVersionControl )
    d = new HgPluginPrivate();

    d->m_factory = new KDevHgViewFactory(this);
    core()->uiController()->addToolView(i18n("Hg"), d->m_factory);

    setXMLFile("kdevhg.rc");
    setupActions();

    d->m_proxy = new HgProxy(this);
}

HgPlugin::~HgPlugin()
{
    delete d;
}


HgProxy* HgPlugin::proxy()
{
    return d->m_proxy;
}

void HgPlugin::setupActions()
{
    KAction *action;

    action = actionCollection()->addAction("hg_init");
    action->setText(i18n("Init Directory..."));
    connect(action, SIGNAL(triggered(bool)), this, SLOT(slotInit()));

//     action = actionCollection()->addAction("hg_clone");
//     action->setText(i18n("Clone..."));
//     connect(action, SIGNAL(triggered(bool)), this, SLOT(slotCheckout()));

//         action = actionCollection()->addAction("cvs_status");
//         action->setText(i18n("Status..."));
//         connect(action, SIGNAL(triggered(bool)), this, SLOT(slotStatus()));
}

void HgPlugin::slotInit()
{
        KUrl url = urlFocusedDocument();

        ImportDialog dlg(this, url);
        dlg.exec();
}

const KUrl HgPlugin::urlFocusedDocument() const
{
    KParts::ReadOnlyPart *plugin =
                         dynamic_cast<KParts::ReadOnlyPart*>( core()->partManager()->activePart() );
    if ( plugin ) {
        if (plugin->url().isLocalFile() ) {
            return plugin->url();
        }
    }
    return KUrl();
}

//     void HgPlugin::slotStatus()
//     {
//         KUrl url = urlFocusedDocument();
//         KUrl::List urls;
//         urls << url;
// 
//         KDevelop::VcsJob* j = status(url, KDevelop::IBasicVersionControl::Recursive);
//         HgJob* job = dynamic_cast<HgJob*>(j);
//         if (job) {
//             HgGenericOutputView* view = new HgGenericOutputView(this, job);
//             emit addNewTabToMainView( view, i18n("Status") );
//             job->start();
//         }
//     }



KDevelop::ContextMenuExtension HgPlugin::contextMenuExtension(KDevelop::Context* context)
{
    KUrl::List ctxUrlList;
    if( context->type() == KDevelop::Context::ProjectItemContext )
    {
        KDevelop::ProjectItemContext *itemCtx = dynamic_cast<KDevelop::ProjectItemContext*>(context);
        if( itemCtx )
        {
            QList<KDevelop::ProjectBaseItem *> baseItemList = itemCtx->items();

            // now general case
            foreach( KDevelop::ProjectBaseItem* _item, baseItemList )
            {
                if( _item->folder() ){
                    KDevelop::ProjectFolderItem *folderItem = dynamic_cast<KDevelop::ProjectFolderItem*>(_item);
                    ctxUrlList << folderItem->url();
                }
                else if( _item->file() ){
                    KDevelop::ProjectFileItem *fileItem = dynamic_cast<KDevelop::ProjectFileItem*>(_item);
                    ctxUrlList << fileItem->url();
                }
            }
        }
    }
    else if( context->type() == KDevelop::Context::EditorContext )
    {
        KDevelop::EditorContext *itemCtx = dynamic_cast<KDevelop::EditorContext*>(context);
        ctxUrlList << itemCtx->url();
    }
    else if( context->type() == KDevelop::Context::FileContext )
    {
        KDevelop::FileContext *itemCtx = dynamic_cast<KDevelop::FileContext*>(context);
        ctxUrlList += itemCtx->urls();
    }

    KDevelop::ContextMenuExtension menuExt;

    bool hasVersionControlledEntries = false;
    foreach(KUrl url, ctxUrlList)
    {
        if(isVersionControlled( url ) )
        {
            hasVersionControlledEntries = true;
            break;
        }
    }
    if(ctxUrlList.isEmpty() )
        return IPlugin::contextMenuExtension(context);


    d->m_ctxUrlList = ctxUrlList;
    QList<QAction*> actions;
    KAction *action;
    kDebug() << "version controlled?" << hasVersionControlledEntries;
    if(hasVersionControlledEntries)
    {
        action = new KAction(i18n("Commit..."), this);
        connect( action, SIGNAL(triggered()), this, SLOT(ctxCommit()) );
        menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );

        action = new KAction(i18n("Add"), this);
        connect( action, SIGNAL(triggered()), this, SLOT(ctxAdd()) );
        menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );

        action = new KAction(i18n("Remove"), this);
        connect( action, SIGNAL(triggered()), this, SLOT(ctxRemove()) );
        menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );

//         action = new KAction(i18n("Update to Head"), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxUpdate()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
// 
//         action = new KAction(i18n("Revert"), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxRevert()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
// 
//         action = new KAction(i18n("Diff to Head"), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxDiffHead()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
// 
//         action = new KAction(i18n("Diff to Base"), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxDiffBase()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
// 
//         action = new KAction(i18n("Copy..."), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxCopy()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
// 
//         action = new KAction(i18n("Move..."), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxMove()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
// 
//         action = new KAction(i18n("History..."), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxHistory()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
// 
//         action = new KAction(i18n("Annotation..."), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxBlame()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
    }
    else
    {
        QMenu* menu = new QMenu("Hg");
        action = new KAction(i18n("Init..."), this);
        connect( action, SIGNAL(triggered()), this, SLOT(slotInit()) );
        menu->addAction( action );
        menuExt.addAction( KDevelop::ContextMenuExtension::ExtensionGroup, menu->menuAction() );
    }

    return menuExt;
}


    void HgPlugin::ctxCommit()
    {
        KDevelop::VcsJob* j = commit("", d->m_ctxUrlList, KDevelop::IBasicVersionControl::Recursive);
        HgJob* job = dynamic_cast<HgJob*>(j);
        if (job) {
            connect(job, SIGNAL( result(KJob*) ),
                    this, SIGNAL( jobFinished(KJob*) ));
            job->start();
        }
    }

    void HgPlugin::ctxAdd()
    {
        KDevelop::VcsJob* j = add(d->m_ctxUrlList, KDevelop::IBasicVersionControl::Recursive);
        HgJob* job = dynamic_cast<HgJob*>(j);
        if (job) {
            connect(job, SIGNAL( result(KJob*) ),
                    this, SIGNAL( jobFinished(KJob*) ));
            job->start();
        }
    }

void HgPlugin::ctxRemove()
{

     KDevelop::VcsJob* j = remove(d->m_ctxUrlList);
      HgJob* job = dynamic_cast<HgJob*>(j);
        if (job) {
          connect(job, SIGNAL( result(KJob*) ),
                            this, SIGNAL( jobFinished(KJob*) ));
           job->start();
      }
         connect(job, SIGNAL( result(KJob*) ),
                           this, SIGNAL( jobFinished(KJob*) ));
         job->start();
    }


//     void HgPlugin::ctxUpdate()
//     {
//         UpdateOptionsDialog dlg;
//         if (dlg.exec() == QDialog::Accepted) {
//             KDevelop::VcsJob* j = update(d->m_ctxUrlList,
//                                          dlg.revision(),
//                                                  KDevelop::IBasicVersionControl::Recursive);
//             HgJob* job = dynamic_cast<HgJob*>(j);
//             if (job) {
//                 connect(job, SIGNAL( result(KJob*) ),
//                         this, SIGNAL( jobFinished(KJob*) ));
//                 job->start();
//             }
//         }
//     }
// 
//     void HgPlugin::ctxLog()
//     {
//         KDevelop::VcsRevision rev;
//         KDevelop::VcsJob* j = log( d->m_ctxUrlList.first(), rev, 0 );
//         HgJob* job = dynamic_cast<HgJob*>(j);
//         if (job) {
//             job->start();
//             LogView* view = new LogView(this, job);
//             emit addNewTabToMainView( view, i18n("Log") );
//         }
//     }
// 
//     void HgPlugin::ctxAnnotate()
//     {
//     /// @todo let user pick annotate revision
//         KDevelop::VcsRevision rev;
// 
//         KDevelop::VcsJob* j = annotate(d->m_ctxUrlList.first(), rev);
//         HgJob* job = dynamic_cast<HgJob*>(j);
//         if (job) {
//             AnnotateView* view = new AnnotateView(this, job);
//             emit addNewTabToMainView( view, i18n("Annotate") );
//             job->start();
//         }
//     }
// 
//     void HgPlugin::ctxRevert()
//     {
//         KDevelop::VcsJob* j = revert(d->m_ctxUrlList,
//                                      KDevelop::IBasicVersionControl::Recursive);
//         HgJob* job = dynamic_cast<HgJob*>(j);
//         if (job) {
//             connect(job, SIGNAL( result(KJob*) ),
//                     this, SIGNAL( jobFinished(KJob*) ));
//             job->start();
//         }
//     }
// 
//     void HgPlugin::ctxDiff()
//     {
//         KUrl url = d->m_ctxUrlList.first();
//         DiffOptionsDialog dlg(0, url);
// 
//         if (dlg.exec() == QDialog::Accepted) {
//             KDevelop::VcsJob* j = diff(url, QString(""), dlg.revA(), dlg.revB(), KDevelop::VcsDiff::DiffUnified);
//             HgJob* job = dynamic_cast<HgJob*>(j);
//             if (job) {
//                 job->start();
//                 HgGenericOutputView* view = new HgGenericOutputView(this, job);
//                 emit addNewTabToMainView( view, i18n("Diff") );
//             }
//         }
//     }
/*
    void HgPlugin::ctxEditors()
    {
    ///@todo find a common base directory for the files
        QFileInfo info( d->m_ctxUrlList[0].toLocalFile() );

        HgJob* job = d->m_proxy->editors( info.absolutePath(),
                                           d->m_ctxUrlList);
        if (job) {
            job->start();
            EditorsView* view = new EditorsView(this, job);
            emit addNewTabToMainView( view, i18n("Editors") );
        }
    }*/




// Begin:  KDevelop::IBasicVersionControl

QString HgPlugin::name() const
{
        return i18n("Hg");
}

KDevelop::VcsImportMetadataWidget* HgPlugin::createImportMetadataWidget( QWidget* parent )
{
    return new ImportMetadataWidget(parent);
}

bool HgPlugin::isVersionControlled( const KUrl& localLocation )
{
    //TODO: some files from repository location can be not version controlled
    return d->m_proxy->isValidDirectory(localLocation);
}

KDevelop::VcsJob * HgPlugin::repositoryLocation(const KUrl & localLocation)
{
    return NULL;
}

//Note: recursion is not used
KDevelop::VcsJob * HgPlugin::add(const KUrl::List & localLocations,
                                  KDevelop::IBasicVersionControl::RecursionMode recursion)
{
    QFileInfo info( localLocations[0].toLocalFile() );

    HgJob* job = d->m_proxy->add(info.absolutePath(), localLocations);
    return job;
}

KDevelop::VcsJob * HgPlugin::remove(const KUrl::List & localLocations)
{
    QFileInfo info(localLocations[0].toLocalFile() );

    HgJob* job = d->m_proxy->remove(info.absolutePath(), localLocations);
    return job;
}

KDevelop::VcsJob * HgPlugin::status(const KUrl::List & localLocations, 
                                     KDevelop::IBasicVersionControl::RecursionMode recursion)
{
    QFileInfo info( localLocations[0].toLocalFile() );
    HgJob* job = d->m_proxy->status( info.absolutePath(),
                                      localLocations,
                                      (recursion == KDevelop::IBasicVersionControl::Recursive) ? true:false);
    return job;
}

///Not used in DVCS;
KDevelop::VcsJob* HgPlugin::copy(const KUrl& localLocationSrc,
                       const KUrl& localLocationDstn) 
{
    return d->m_proxy->empty_cmd();
}

///Not used in DVCS;
KDevelop::VcsJob* HgPlugin::move(const KUrl& localLocationSrc,
                       const KUrl& localLocationDst ) 
{
    return d->m_proxy->empty_cmd();
}

KDevelop::VcsJob * HgPlugin::revert(const KUrl::List & localLocations, 
                                     KDevelop::IBasicVersionControl::RecursionMode recursion)
{
    return d->m_proxy->empty_cmd();
}

KDevelop::VcsJob * HgPlugin::update(const KUrl::List & localLocations, const KDevelop::VcsRevision & rev,
                                     KDevelop::IBasicVersionControl::RecursionMode recursion)
{
    return d->m_proxy->empty_cmd();
}

KDevelop::VcsJob * HgPlugin::commit(const QString & message, const KUrl::List & localLocations,
                                     KDevelop::IBasicVersionControl::RecursionMode recursion)
{
    QString msg = message;
    if( msg.isEmpty() )
    {
        CommitDialog dlg;
        if( dlg.exec() == QDialog::Accepted )
        {
            msg = dlg.message();
        }
    }
    QFileInfo info( localLocations[0].toLocalFile() );

    HgJob* job = d->m_proxy->commit(info.absolutePath(), msg, localLocations);
    return job;
}

KDevelop::VcsJob * HgPlugin::diff(const KDevelop::VcsLocation & localOrRepoLocationSrc,
                                   const KDevelop::VcsLocation & localOrRepoLocationDst,
                                   const KDevelop::VcsRevision & srcRevision,
                                   const KDevelop::VcsRevision & dstRevision,
                                   KDevelop::VcsDiff::Type, KDevelop::IBasicVersionControl::RecursionMode)
{
    return d->m_proxy->empty_cmd();
}






KDevelop::VcsJob* HgPlugin::log(const KUrl& localLocation,
                      const KDevelop::VcsRevision& rev,
                      unsigned long limit )
{
    return d->m_proxy->empty_cmd();
}

KDevelop::VcsJob* HgPlugin::log(const KUrl& localLocation,
                      const KDevelop::VcsRevision& rev,
                      const KDevelop::VcsRevision& limit )
{
    return d->m_proxy->empty_cmd();
}

KDevelop::VcsJob* HgPlugin::annotate(const KUrl& localLocation,
                           const KDevelop::VcsRevision& rev )
{
    return d->m_proxy->empty_cmd();
}

KDevelop::VcsJob* HgPlugin::merge(const KDevelop::VcsLocation& localOrRepoLocationSrc,
                        const KDevelop::VcsLocation& localOrRepoLocationDst,
                        const KDevelop::VcsRevision& srcRevision,
                        const KDevelop::VcsRevision& dstRevision,
                        const KUrl& localLocation )
{
    return d->m_proxy->empty_cmd();
}

KDevelop::VcsJob* HgPlugin::resolve(const KUrl::List& localLocations,
                          KDevelop::IBasicVersionControl::RecursionMode recursion )
{
    return d->m_proxy->empty_cmd();
}

// End:  KDevelop::IBasicVersionControl


// Begin:  KDevelop::IDistributedVersionControl
KDevelop::VcsJob* HgPlugin::init(const KUrl& localRepositoryRoot)
{
    HgJob* job = d->m_proxy->init(localRepositoryRoot);
    return job;
}

KDevelop::VcsJob* HgPlugin::clone(const QString& repositoryLocationSrc,
                        const KUrl& localRepositoryRoot )
{
    return d->m_proxy->empty_cmd();
}

KDevelop::VcsJob* HgPlugin::push(const KUrl& localRepositoryLocation,
                       const QString& repositoryLocation )
{
    return d->m_proxy->empty_cmd();
}

KDevelop::VcsJob* HgPlugin::pull(const QString& repositoryLocation,
                       const KUrl& localRepositoryLocation )
{
    return d->m_proxy->empty_cmd();
}

// End:  KDevelop::IDistributedVersionControl


// #include "hgplugin.moc"
