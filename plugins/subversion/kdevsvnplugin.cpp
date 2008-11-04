/***************************************************************************
 *   Copyright 2007 Dukju Ahn <dukjuahn@gmail.com>                         *
 *   Copyright 2008 Andreas Pakulat <apaku@gmx.de>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "kdevsvnplugin.h"

#include <QtDesigner/QExtensionFactory>
#include <QAction>
#include <QVariant>
#include <QTextStream>
#include <QMenu>

#include <kparts/part.h>
#include <kparts/partmanager.h>
#include <kparts/mainwindow.h>
#include <kaboutdata.h>
#include <ktexteditor/document.h>
#include <ktexteditor/markinterface.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <klocale.h>
#include <kurlrequester.h>
#include <kaction.h>
#include <kurlrequesterdialog.h>
#include <kfile.h>
#include <ktemporaryfile.h>
#include <kmessagebox.h>

#include <interfaces/iuicontroller.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/idocument.h>
#include <interfaces/iplugincontroller.h>
#include <interfaces/icore.h>
#include <interfaces/iruncontroller.h>
#include <project/projectmodel.h>
#include <interfaces/context.h>
#include <interfaces/contextmenuextension.h>
#include <vcs/vcsrevision.h>
#include <vcs/vcsevent.h>
#include <vcs/vcsrevision.h>
#include <vcs/vcsmapping.h>
#include <vcs/vcsstatusinfo.h>
#include <vcs/vcsannotation.h>
#include <vcs/widgets/vcsannotationwidget.h>
#include <vcs/widgets/vcseventwidget.h>
#include <vcs/widgets/vcsdiffwidget.h>
#include <vcs/widgets/vcscommitdialog.h>
#include <language/interfaces/editorcontext.h>

#include "kdevsvncpp/apr.hpp"

#include "svncommitjob.h"
#include "svnstatusjob.h"
#include "svnaddjob.h"
#include "svnrevertjob.h"
#include "svnremovejob.h"
#include "svnupdatejob.h"
#include "svninfojob.h"
#include "svndiffjob.h"
#include "svncopyjob.h"
#include "svnmovejob.h"
#include "svnlogjob.h"
#include "svnblamejob.h"
#include "svnimportjob.h"
#include "svncheckoutjob.h"

#include "svnimportmetadatawidget.h"
#include "svncheckoutmetadatawidget.h"

K_PLUGIN_FACTORY(KDevSvnFactory, registerPlugin<KDevSvnPlugin>(); )
K_EXPORT_PLUGIN(KDevSvnFactory(KAboutData("kdevsubversion","kdevsubversion", ki18n("Subversion"), "0.1", ki18n("Support for Subversion version control systems"), KAboutData::License_GPL)))

KDevSvnPlugin::KDevSvnPlugin( QObject *parent, const QVariantList & )
    : KDevelop::IPlugin(KDevSvnFactory::componentData(), parent)
{
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IBasicVersionControl)
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::ICentralizedVersionControl )

    qRegisterMetaType<KDevelop::VcsStatusInfo>();
    qRegisterMetaType<SvnInfoHolder>();
    qRegisterMetaType<KDevelop::VcsEvent>();
    qRegisterMetaType<KDevelop::VcsRevision>();
    qRegisterMetaType<KDevelop::VcsRevision::RevisionSpecialType>();
    qRegisterMetaType<KDevelop::VcsAnnotation>();
    qRegisterMetaType<KDevelop::VcsAnnotationLine>();
}

KDevSvnPlugin::~KDevSvnPlugin()
{
}

bool KDevSvnPlugin::isVersionControlled( const KUrl& localLocation )
{
    SvnInfoJob* job = new SvnInfoJob( this );

    job->setLocation( localLocation );
    if( job->exec() )
    {
        QVariant result = job->fetchResults();
        if( result.isValid() )
        {
            SvnInfoHolder h = qVariantValue<SvnInfoHolder>( result );
            return !h.name.isEmpty();
        }
    }else
    {
        kDebug(9510) << "Couldn't execute job";
    }
    return false;
}

KDevelop::VcsJob* KDevSvnPlugin::repositoryLocation( const KUrl& localLocation )
{
    SvnInfoJob* job = new SvnInfoJob( this );

    job->setLocation( localLocation );
    job->setProvideInformation( SvnInfoJob::RepoUrlOnly );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::status( const KUrl::List& localLocations,
                                    KDevelop::IBasicVersionControl::RecursionMode mode )
{
    SvnStatusJob* job = new SvnStatusJob( this );
    job->setLocations( localLocations );
    job->setRecursive( ( mode == KDevelop::IBasicVersionControl::Recursive ) );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::add( const KUrl::List& localLocations,
                                      KDevelop::IBasicVersionControl::RecursionMode recursion )
{
    SvnAddJob* job = new SvnAddJob( this );
    job->setLocations( localLocations );
    job->setRecursive( ( recursion == KDevelop::IBasicVersionControl::Recursive ) );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::remove( const KUrl::List& localLocations )
{
    SvnRemoveJob* job = new SvnRemoveJob( this );
    job->setLocations( localLocations );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::edit( const KUrl& /*localLocation*/ )
{
    return 0;
}

KDevelop::VcsJob* KDevSvnPlugin::unedit( const KUrl& /*localLocation*/ )
{
    return 0;
}

KDevelop::VcsJob* KDevSvnPlugin::localRevision( const KUrl& localLocation, KDevelop::VcsRevision::RevisionType type )
{
    SvnInfoJob* job = new SvnInfoJob( this );

    job->setLocation( localLocation );
    job->setProvideInformation( SvnInfoJob::RevisionOnly );
    job->setProvideRevisionType( type );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::copy( const KUrl& localLocationSrc, const KUrl& localLocationDstn )
{
    SvnCopyJob* job = new SvnCopyJob( this );
    job->setSourceLocation( localLocationSrc );
    job->setDestinationLocation( localLocationDstn );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::move( const KUrl& localLocationSrc, const KUrl& localLocationDst )
{
    SvnMoveJob* job = new SvnMoveJob( this );
    job->setSourceLocation( localLocationSrc );
    job->setDestinationLocation( localLocationDst );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::revert( const KUrl::List& localLocations,
                                               KDevelop::IBasicVersionControl::RecursionMode recursion )
{
    SvnRevertJob* job = new SvnRevertJob( this );
    job->setLocations( localLocations );
    job->setRecursive( ( recursion == KDevelop::IBasicVersionControl::Recursive ) );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::update( const KUrl::List& localLocations,
                                    const KDevelop::VcsRevision& rev,
                                    KDevelop::IBasicVersionControl::RecursionMode recursion )
{
    SvnUpdateJob* job = new SvnUpdateJob( this );
    job->setLocations( localLocations );
    job->setRevision( rev );
    job->setRecursive( ( recursion == KDevelop::IBasicVersionControl::Recursive ) );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::commit( const QString& message, const KUrl::List& localLocations,
                    KDevelop::IBasicVersionControl::RecursionMode recursion )
{
    SvnCommitJob* job = new SvnCommitJob( this );
    kDebug(9510) << "Committing locations:" << localLocations << endl;
    job->setUrls( localLocations );
    job->setCommitMessage( message ) ;
    job->setRecursive( ( recursion == KDevelop::IBasicVersionControl::Recursive ) );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::diff( const KDevelop::VcsLocation& src,
                const KDevelop::VcsLocation& dst,
                const KDevelop::VcsRevision& srcRevision,
                const KDevelop::VcsRevision& dstRevision,
                KDevelop::VcsDiff::Type diffType,
                KDevelop::IBasicVersionControl::RecursionMode recurse )
{
    SvnDiffJob* job = new SvnDiffJob( this );
    job->setSource( src );
    job->setDestination( dst );
    job->setSrcRevision( srcRevision );
    job->setDstRevision( dstRevision );
    job->setDiffType( diffType );
    job->setRecursive( ( recurse == KDevelop::IBasicVersionControl::Recursive ) );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::log( const KUrl& localLocation, const KDevelop::VcsRevision& rev, unsigned long limit )
{
    SvnLogJob* job = new SvnLogJob( this );
    job->setLocation( localLocation );
    job->setStartRevision( rev );
    job->setLimit( limit );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::log( const KUrl& localLocation,
            const KDevelop::VcsRevision& startRev,
            const KDevelop::VcsRevision& endRev )
{
    SvnLogJob* job = new SvnLogJob( this );
    job->setLocation( localLocation );
    job->setStartRevision( startRev );
    job->setEndRevision( endRev );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::annotate( const KUrl& localLocation,
                const KDevelop::VcsRevision& rev )
{
    SvnBlameJob* job = new SvnBlameJob( this );
    job->setLocation( localLocation );
    job->setEndRevision( rev );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::merge( const KDevelop::VcsLocation& localOrRepoLocationSrc,
            const KDevelop::VcsLocation& localOrRepoLocationDst,
            const KDevelop::VcsRevision& srcRevision,
            const KDevelop::VcsRevision& dstRevision,
            const KUrl& localLocation )
{
    // TODO implement merge
    Q_UNUSED( localOrRepoLocationSrc )
    Q_UNUSED( localOrRepoLocationDst )
    Q_UNUSED( srcRevision )
    Q_UNUSED( dstRevision )
    Q_UNUSED( localLocation )
    return 0;
}

KDevelop::VcsJob* KDevSvnPlugin::resolve( const KUrl::List& /*localLocations*/,
                KDevelop::IBasicVersionControl::RecursionMode /*recursion*/ )
{
    return 0;
}

KDevelop::VcsJob* KDevSvnPlugin::import( const KDevelop::VcsMapping& mapping, const QString& msg )
{
    SvnImportJob* job = new SvnImportJob( this );
    job->setMapping( mapping );
    job->setMessage( msg );
    return job;
}

KDevelop::VcsJob* KDevSvnPlugin::checkout( const KDevelop::VcsMapping& mapping )
{
    SvnCheckoutJob* job = new SvnCheckoutJob( this );
    job->setMapping( mapping );
    return job;
}


KDevelop::ContextMenuExtension KDevSvnPlugin::contextMenuExtension( KDevelop::Context* context )
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
    }else if( context->type() == KDevelop::Context::EditorContext )
    {
        KDevelop::EditorContext *itemCtx = dynamic_cast<KDevelop::EditorContext*>(context);
        ctxUrlList << itemCtx->url();
    }else if( context->type() == KDevelop::Context::FileContext )
    {
        KDevelop::FileContext *itemCtx = dynamic_cast<KDevelop::FileContext*>(context);
        ctxUrlList += itemCtx->urls();
    }


    KDevelop::ContextMenuExtension menuExt;

    bool hasVersionControlledEntries = false;
    foreach( KUrl url, ctxUrlList )
    {
        if( isVersionControlled( url ) )
        {
            hasVersionControlledEntries = true;
            break;
        }
    }
    if( ctxUrlList.isEmpty() )
        return IPlugin::contextMenuExtension( context );


    m_ctxUrlList = ctxUrlList;
    QList<QAction*> actions;
    KAction *action;
    QMenu* menu = new QMenu("Subversion");
    kDebug() << "version controlled?" << hasVersionControlledEntries;
    if( hasVersionControlledEntries )
    {
//         action = new KAction(i18n("Commit..."), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxCommit()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
//
//         action = new KAction(i18n("Add to Repository"), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxAdd()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
//
//         action = new KAction(i18n("Remove from Repository"), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxRemove()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
//
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
        action = new KAction(i18n("Copy..."), this);
        connect( action, SIGNAL(triggered()), this, SLOT(ctxCopy()) );
        menu->addAction( action );

        action = new KAction(i18n("Move..."), this);
        connect( action, SIGNAL(triggered()), this, SLOT(ctxMove()) );
        menu->addAction( action );
//
//         action = new KAction(i18n("History..."), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxHistory()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
//
//         action = new KAction(i18n("Annotation..."), this);
//         connect( action, SIGNAL(triggered()), this, SLOT(ctxBlame()) );
//         menuExt.addAction( KDevelop::ContextMenuExtension::VcsGroup, action );
    }
    menuExt.addAction( KDevelop::ContextMenuExtension::ExtensionGroup, menu->menuAction() );

    return menuExt;
}

void KDevSvnPlugin::ctxHistory()
{
    if( m_ctxUrlList.count() > 1 ){
        KMessageBox::error( 0, i18n("Please select only one item for this operation") );
        return;
    }
    KDevelop::VcsRevision start;
    start.setRevisionValue( qVariantFromValue<KDevelop::VcsRevision::RevisionSpecialType>( KDevelop::VcsRevision::Head ),
                            KDevelop::VcsRevision::Special );
    KDevelop::VcsJob *job = log( m_ctxUrlList.first(), start, 0 );
    KDialog* dlg = new KDialog();
    dlg->setButtons( KDialog::Close );
    dlg->setCaption( i18n( "Subversion Log (%1)", m_ctxUrlList.first().path() ) );
    KDevelop::VcsEventWidget* logWidget = new KDevelop::VcsEventWidget( m_ctxUrlList.first(), job, dlg );
    dlg->setMainWidget( logWidget );
    connect( dlg, SIGNAL( destroyed( QObject* ) ), job, SLOT( deleteLater() ) );
    dlg->show();
}
void KDevSvnPlugin::ctxBlame()
{
    if( m_ctxUrlList.count() > 1 ){
        KMessageBox::error( 0, i18n("Please select only one item for this operation") );
        return;
    }

    if( !m_ctxUrlList.first().isLocalFile() )
    {
        KMessageBox::error( 0, i18n("Annotate is only supported on local files") );
        return;
    }

    KDevelop::IDocument* doc = core()->documentController()->documentForUrl( m_ctxUrlList.first() );
    if( !doc )
        doc = core()->documentController()->openDocument( m_ctxUrlList.first() );

    if( doc && doc->textDocument() )
    {
        KDevelop::VcsRevision head;
        head.setRevisionValue( qVariantFromValue<KDevelop::VcsRevision::RevisionSpecialType>( KDevelop::VcsRevision::Head ),
                            KDevelop::VcsRevision::Special );
        KDevelop::VcsJob* job = annotate( m_ctxUrlList.first(), head );
        KTextEditor::MarkInterface* markiface = 0;
                //qobject_cast<KTextEditor::MarkInterface*>(doc->textDocument());
        if( markiface )
        {
            //@TODO: Work with Kate devs towards a new interface for adding
            //       annotation information to the KTE's in KDE 4.1
        }else
        {
            KDialog* dlg = new KDialog();
            dlg->setButtons( KDialog::Close );
            dlg->setCaption( i18n("Annotation (%1)", m_ctxUrlList.first().prettyUrl() ) );
            KDevelop::VcsAnnotationWidget* w = new KDevelop::VcsAnnotationWidget( m_ctxUrlList.first(), job, dlg );
            dlg->setMainWidget( w );
            connect( dlg, SIGNAL( destroyed( QObject* ) ), job, SLOT( deleteLater() ) );
            dlg->show();
        }
    }else
    {
        KMessageBox::error( 0, i18n("Cannot execute annotate action because the "
                                    "document wasn't found or was not a text "
                                    "document:\n%1", m_ctxUrlList.first().prettyUrl() ) );
    }

}
void KDevSvnPlugin::ctxCommit()
{
    if( !m_ctxUrlList.isEmpty() )
    {
        KDevelop::VcsCommitDialog* dlg = new KDevelop::VcsCommitDialog( this, core()->uiController()->activeMainWindow() );
        dlg->setCommitCandidates( m_ctxUrlList );
        dlg->setRecursive( true );
        connect( dlg, SIGNAL( doCommit( KDevelop::VcsCommitDialog* ) ), this, SLOT( doCommit( KDevelop::VcsCommitDialog* ) ) );
        connect( dlg, SIGNAL( cancelCommit( KDevelop::VcsCommitDialog* ) ), this, SLOT( cancelCommit( KDevelop::VcsCommitDialog* ) ) );
        dlg->show();
    }
}


void KDevSvnPlugin::doCommit( KDevelop::VcsCommitDialog* dlg )
{
    KDevelop::IBasicVersionControl::RecursionMode mode;
    if( dlg->recursive() )
    {
        mode = KDevelop::IBasicVersionControl::Recursive;
    }else
    {
        mode = KDevelop::IBasicVersionControl::NonRecursive;
    }
    KDevelop::ICore::self()->runController()->registerJob( commit( dlg->message(), dlg->checkedUrls(), mode ) );
    dlg->deleteLater();
}

void KDevSvnPlugin::cancelCommit( KDevelop::VcsCommitDialog* dlg )
{
    dlg->deleteLater();
}

void KDevSvnPlugin::ctxUpdate()
{
    KDevelop::VcsRevision rev;
    rev.setRevisionValue( qVariantFromValue<KDevelop::VcsRevision::RevisionSpecialType>( KDevelop::VcsRevision::Head ), KDevelop::VcsRevision::Special );
    KDevelop::ICore::self()->runController()->registerJob( update( m_ctxUrlList, rev ) );
}

void KDevSvnPlugin::ctxAdd()
{
    KDevelop::ICore::self()->runController()->registerJob( add( m_ctxUrlList ) );
}
void KDevSvnPlugin::ctxRemove()
{
    KDevelop::ICore::self()->runController()->registerJob( remove( m_ctxUrlList ) );
}

void KDevSvnPlugin::ctxRevert()
{
    //@TODO: If one of the urls is a directory maybe ask whether all files in the dir should be reverted?
    KDevelop::ICore::self()->runController()->registerJob( revert( m_ctxUrlList ) );
}

void KDevSvnPlugin::ctxDiff()
{
    // TODO correct port
    if( m_ctxUrlList.count() > 1 ){
        KMessageBox::error( 0, i18n("Please select only one item for this operation") );
        return;
    }
}

void KDevSvnPlugin::ctxDiffHead()
{
    if( m_ctxUrlList.count() > 1 ){
        KMessageBox::error( 0, i18n("Please select only one item for this operation") );
        return;
    }
    KDevelop::VcsRevision srcRev,dstRev;
    srcRev.setRevisionValue( qVariantFromValue<KDevelop::VcsRevision::RevisionSpecialType>(KDevelop::VcsRevision::Head), KDevelop::VcsRevision::Special );
    dstRev.setRevisionValue( qVariantFromValue<KDevelop::VcsRevision::RevisionSpecialType>(KDevelop::VcsRevision::Working), KDevelop::VcsRevision::Special );
    KDevelop::VcsJob* job = diff( m_ctxUrlList.first(), m_ctxUrlList.first(), srcRev, dstRev );

    //TODO: Fix this, the job should execute asynchronously via runcontroller
    job->exec();
    if( job->status() == KDevelop::VcsJob::JobSucceeded )
    {
        KDevelop::VcsDiff d = job->fetchResults().value<KDevelop::VcsDiff>();
        QString diff = d.diff();
        core()->documentController()->openDocumentFromText( diff );
    }else
    {
        kDebug(9510) << "Ooops couldn't diff";
    }
    delete job;
}
void KDevSvnPlugin::ctxDiffBase()
{
    if( m_ctxUrlList.count() > 1 ){
        KMessageBox::error( 0, i18n("Please select only one item for this operation") );
        return;
    }
    KDevelop::VcsRevision srcRev,dstRev;
    srcRev.setRevisionValue( qVariantFromValue<KDevelop::VcsRevision::RevisionSpecialType>(KDevelop::VcsRevision::Base), KDevelop::VcsRevision::Special );
    dstRev.setRevisionValue( qVariantFromValue<KDevelop::VcsRevision::RevisionSpecialType>(KDevelop::VcsRevision::Working), KDevelop::VcsRevision::Special );
    KDevelop::VcsJob* job = diff( m_ctxUrlList.first(), m_ctxUrlList.first(), srcRev, dstRev );

    //TODO: same as above ctxDiffHead
    job->exec();
    if( job->status() == KDevelop::VcsJob::JobSucceeded )
    {
        KDevelop::VcsDiff d = job->fetchResults().value<KDevelop::VcsDiff>();
        QString diff = d.diff();
        core()->documentController()->openDocumentFromText( diff );
    }else
    {
        kDebug(9510) << "Ooops couldn't diff";
    }
    delete job;
}
void KDevSvnPlugin::ctxInfo()
{
    if( m_ctxUrlList.count() > 1 ){
        KMessageBox::error( 0, i18n("Please select only one item for this operation") );
        return;
    }
}
void KDevSvnPlugin::ctxStatus()
{
    if( m_ctxUrlList.count() > 1 ){
        KMessageBox::error( 0, i18n("Please select only one item for this operation") );
        return;
    }
}

void KDevSvnPlugin::ctxCopy()
{
    if( m_ctxUrlList.count() > 1 ){
        KMessageBox::error( 0, i18n("Please select only one item for this operation") );
        return;
    }
    KUrl source = m_ctxUrlList.first();
    if( source.isLocalFile() )
    {
        QString dir = source.path();
        bool isFile = QFileInfo( source.path() ).isFile();
        if( isFile )
        {
            dir = source.directory();
        }
        KUrlRequesterDialog dlg( dir, i18n("Destination file/directory"), 0 );
        if( isFile )
        {
            dlg.urlRequester()->setMode( KFile::File | KFile::Directory | KFile::LocalOnly );
        }else
        {
            dlg.urlRequester()->setMode( KFile::Directory | KFile::LocalOnly );
        }
        if( dlg.exec() == QDialog::Accepted )
        {
            KDevelop::ICore::self()->runController()->registerJob( copy( source, dlg.selectedUrl() ) );
        }
    }else
    {
        KMessageBox::error( 0, i18n("Copying only works on local files") );
        return;
    }

}

void KDevSvnPlugin::ctxMove()
{
    if( m_ctxUrlList.count() > 1 ){
        KMessageBox::error( 0, i18n("Please select only one item for this operation") );
        return;
    }
        KUrl source = m_ctxUrlList.first();
    if( source.isLocalFile() )
    {
        QString dir = source.path();
        bool isFile = QFileInfo( source.path() ).isFile();
        if( isFile )
        {
            dir = source.directory();
        }
        KUrlRequesterDialog dlg( dir, i18n("Destination file/directory"), 0 );
        if( isFile )
        {
            dlg.urlRequester()->setMode( KFile::File | KFile::Directory | KFile::LocalOnly );
        }else
        {
            dlg.urlRequester()->setMode( KFile::Directory | KFile::LocalOnly );
        }
        if( dlg.exec() == QDialog::Accepted )
        {
            KDevelop::ICore::self()->runController()->registerJob( move( source, dlg.selectedUrl() ) );
        }
    }else
    {
        KMessageBox::error( 0, i18n("Moving only works on local files/dirs") );
        return;
    }
}

void KDevSvnPlugin::ctxCat()
{
    if( m_ctxUrlList.count() > 1 ){
        KMessageBox::error( 0, i18n("Please select only one item for this operation") );
        return;
    }
}

QString KDevSvnPlugin::name() const
{
    return i18n("Subversion");
}

KDevelop::VcsImportMetadataWidget* KDevSvnPlugin::createImportMetadataWidget( QWidget* parent )
{
    return new SvnImportMetadataWidget( parent );
}

void KDevSvnPlugin::ctxImport( )
{
    if( m_ctxUrlList.count() > 1 ){
        KMessageBox::error( 0, i18n("Please select only one item for this operation") );
        return;
    }
    KDialog dlg;
    dlg.setCaption(i18n("Import into Subversion repository"));
    SvnImportMetadataWidget* widget = new SvnImportMetadataWidget(&dlg);
    widget->setSourceLocation( KDevelop::VcsLocation( m_ctxUrlList.first() ) );
    widget->setSourceLocationEditable( false );
    dlg.setMainWidget( widget );
    if( dlg.exec() == QDialog::Accepted )
    {
        KDevelop::ICore::self()->runController()->registerJob( import( widget->mapping(), widget->message() ) );
    }
}

void KDevSvnPlugin::ctxCheckout( )
{
    if( m_ctxUrlList.count() > 1 ){
        KMessageBox::error( 0, i18n("Please select only one item for this operation") );
        return;
    }
    KDialog dlg;
    dlg.setCaption(i18n("Checkout from Subversion repository"));
    SvnCheckoutMetadataWidget* widget = new SvnCheckoutMetadataWidget(&dlg);
    KUrl tmp = m_ctxUrlList.first();
    tmp.cd("..");
    widget->setDestinationLocation( tmp );
    dlg.setMainWidget( widget );
    if( dlg.exec() == QDialog::Accepted )
    {
        KDevelop::ICore::self()->runController()->registerJob( checkout( widget->mapping() ) );
    }
}

#include "kdevsvnplugin.moc"

