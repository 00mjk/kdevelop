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

#ifndef KDEVSVNPLUGIN_H
#define KDEVSVNPLUGIN_H

#include <vcs/interfaces/icentralizedversioncontrol.h>
#include <vcs/vcsdiff.h>
#include <vcs/vcslocation.h>
#include <interfaces/iplugin.h>


class QMenu;
class KUrl;
class SvnStatusHolder;
class KTempDir;
class SvnOutputDelegate;
class SvnOutputModel;
class SvnCommitDialog;

namespace KDevelop
{
class VcsCommitDialog;
class ContextMenuExtension;
}

class KDevSvnPlugin: public KDevelop::IPlugin, public KDevelop::ICentralizedVersionControl
{
    Q_OBJECT
    Q_INTERFACES( KDevelop::IBasicVersionControl KDevelop::ICentralizedVersionControl )
public:
    KDevSvnPlugin( QObject *parent, const QVariantList & = QVariantList() );
    virtual ~KDevSvnPlugin();

    virtual QString name() const;
    virtual KDevelop::VcsImportMetadataWidget* createImportMetadataWidget( QWidget* parent );

    // Begin:  KDevelop::IBasicVersionControl
    bool isVersionControlled( const KUrl& localLocation );

    KDevelop::VcsJob* repositoryLocation( const KUrl& localLocation );

    KDevelop::VcsJob* status( const KUrl::List& localLocations,
                              KDevelop::IBasicVersionControl::RecursionMode recursion
                                      = KDevelop::IBasicVersionControl::Recursive );

    KDevelop::VcsJob* add( const KUrl::List& localLocations,
                           KDevelop::IBasicVersionControl::RecursionMode recursion
                                      = KDevelop::IBasicVersionControl::Recursive );

    KDevelop::VcsJob* remove( const KUrl::List& localLocations );

    KDevelop::VcsJob* copy( const KUrl& localLocationSrc,
                            const KUrl& localLocationDstn );

    KDevelop::VcsJob* move( const KUrl& localLocationSrc,
                            const KUrl& localLocationDst );

    KDevelop::VcsJob* revert( const KUrl::List& localLocations,
                            KDevelop::IBasicVersionControl::RecursionMode recursion
                                      = KDevelop::IBasicVersionControl::Recursive );

    KDevelop::VcsJob* update( const KUrl::List& localLocations,
                            const KDevelop::VcsRevision& rev,
                            KDevelop::IBasicVersionControl::RecursionMode recursion
                                      = KDevelop::IBasicVersionControl::Recursive );

    KDevelop::VcsJob* commit( const QString& message,
                            const KUrl::List& localLocations,
                            KDevelop::IBasicVersionControl::RecursionMode recursion
                                      = KDevelop::IBasicVersionControl::Recursive );

    KDevelop::VcsJob* diff( const KDevelop::VcsLocation& localOrRepoLocationSrc,
                            const KDevelop::VcsLocation& localOrRepoLocationDst,
                            const KDevelop::VcsRevision& srcRevision,
                            const KDevelop::VcsRevision& dstRevision,
                            KDevelop::VcsDiff::Type = KDevelop::VcsDiff::DiffDontCare,
                            KDevelop::IBasicVersionControl::RecursionMode = KDevelop::IBasicVersionControl::Recursive );

    KDevelop::VcsJob* log( const KUrl& localLocation,
                         const KDevelop::VcsRevision& rev,
                         unsigned long limit );

    KDevelop::VcsJob* log( const KUrl& localLocation,
                         const KDevelop::VcsRevision& rev,
                         const KDevelop::VcsRevision& limit );

    KDevelop::VcsJob* annotate( const KUrl& localLocation,
                              const KDevelop::VcsRevision& rev );

    KDevelop::VcsJob* merge( const KDevelop::VcsLocation& localOrRepoLocationSrc,
                           const KDevelop::VcsLocation& localOrRepoLocationDst,
                           const KDevelop::VcsRevision& srcRevision,
                           const KDevelop::VcsRevision& dstRevision,
                           const KUrl& localLocation );

    KDevelop::VcsJob* resolve( const KUrl::List& localLocations,
                             KDevelop::IBasicVersionControl::RecursionMode recursion );
    // End:  KDevelop::IBasicVersionControl

    // Begin:  KDevelop::ICentralizedVersionControl
    KDevelop::VcsJob* import( const KDevelop::VcsMapping& localLocation, const QString& commitMessage );

    KDevelop::VcsJob* checkout( const KDevelop::VcsMapping & mapping );

    KDevelop::VcsJob* edit( const KUrl& localLocation );

    KDevelop::VcsJob* unedit( const KUrl& localLocation );

    KDevelop::VcsJob* localRevision( const KUrl& localLocation,
                                     KDevelop::VcsRevision::RevisionType );
    // End:  KDevelop::ICentralizedVersionControl

public:
    const KUrl urlFocusedDocument( );

    SvnOutputModel* outputModel() const;

    KDevelop::ContextMenuExtension contextMenuExtension( KDevelop::Context* );

public Q_SLOTS:

    // invoked by context-menu
    void ctxHistory();
    void ctxBlame();
    void ctxCommit();
    void ctxUpdate();
    void ctxAdd();
    void ctxRemove();
    void ctxRevert();
    void ctxDiff();
    void ctxDiffHead();
    void ctxDiffBase();
    void ctxInfo();
    void ctxStatus();
    void ctxCopy();
    void ctxMove();
    void ctxCat();
    void ctxImport();
    void ctxCheckout();
private slots:
    void doCommit( KDevelop::VcsCommitDialog* );
    void cancelCommit( KDevelop::VcsCommitDialog* );
private:
    SvnOutputModel* m_outputmodel;
    SvnOutputDelegate* m_outputdelegate;
    class KDevSvnViewFactory *m_factory;
    KUrl::List m_ctxUrlList;
};
#endif

