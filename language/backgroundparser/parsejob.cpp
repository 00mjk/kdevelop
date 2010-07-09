/*
* This file is part of KDevelop
*
* Copyright 2006 Adam Treat <treat@kde.org>
* Copyright 2006-2008 Hamish Rodda <rodda@kde.org>
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

#include "parsejob.h"

#include <cassert>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <QFile>
#include <QByteArray>
#include <QMutex>
#include <QMutexLocker>

#include <kdebug.h>
#include <klocale.h>

#include <ktexteditor/document.h>
#include <ktexteditor/smartinterface.h>

#include "backgroundparser.h"
#include "parserdependencypolicy.h"
#include "duchain/topducontext.h"

#include "duchain/duchainlock.h"
#include "duchain/duchain.h"
#include "duchain/parsingenvironment.h"
#include <interfaces/foregroundlock.h>
#include <interfaces/icore.h>
#include <interfaces/ilanguagecontroller.h>
#include <codegen/coderepresentation.h>
#include <ktexteditor/movinginterface.h>
#include <ktexteditor/view.h>
#include <QApplication>
#include <duchain/declaration.h>
#include <duchain/use.h>


using namespace KTextEditor;

Q_DECLARE_METATYPE(KDevelop::IndexedString)
Q_DECLARE_METATYPE(KDevelop::IndexedTopDUContext)
Q_DECLARE_METATYPE(KDevelop::ReferencedTopDUContext)

static QMutex minimumFeaturesMutex;
static QHash<KDevelop::IndexedString, QList<KDevelop::TopDUContext::Features> > staticMinimumFeatures;

namespace KDevelop
{

class ParseJobPrivate
{
public:

    ParseJobPrivate(const KUrl& url) :
          document( IndexedString(url.pathOrUrl()) )
        , backgroundParser( 0 )
        , abortMutex(new QMutex)
        , hasReadContents( false )
        , abortRequested( false )
        , aborted( false )
        , features( TopDUContext::VisibleDeclarationsAndContexts )
        , previousDocumentRevision( -1 )
    {
    }

    ~ParseJobPrivate()
    {
        delete abortMutex;
    }

    ReferencedTopDUContext duContext;

    KDevelop::IndexedString document;
    QString errorMessage;
    BackgroundParser* backgroundParser;

    QMutex* abortMutex;

    ParseJob::Contents contents;
    
    bool hasReadContents : 1;
    volatile bool abortRequested : 1;
    bool aborted : 1;
    TopDUContext::Features features;
    QList<QPointer<QObject> > notify;
    QPointer<DocumentChangeTracker> tracker;
    RevisionReference revision;
    RevisionReference previousRevision;
    
    // Document revision at the previous parse run, as retrieved from the tracker.
    qint64 previousDocumentRevision;
};

ParseJob::ParseJob( const KUrl &url )
        : ThreadWeaver::JobSequence(),
        d(new ParseJobPrivate(url))
{}

void ParseJob::setTracker ( DocumentChangeTracker* tracker )
{
    d->tracker = tracker;
}

DocumentChangeTracker* ParseJob::tracker() const
{
    return d->tracker;
}

ParseJob::~ParseJob()
{
    typedef QPointer<QObject> QObjectPointer;
    foreach(const QObjectPointer &p, d->notify)
        if(p)
            QMetaObject::invokeMethod(p, "updateReady", Qt::QueuedConnection, Q_ARG(KDevelop::IndexedString, d->document), Q_ARG(KDevelop::ReferencedTopDUContext, d->duContext));

    delete d;
}

IndexedString ParseJob::document() const
{
    return d->document;
}

bool ParseJob::success() const
{
    return !d->aborted;
}

void ParseJob::setMinimumFeatures(TopDUContext::Features features)
{
    d->features = features;
}

bool ParseJob::hasStaticMinimumFeatures()
{
    QMutexLocker lock(&minimumFeaturesMutex);
    return ::staticMinimumFeatures.size();
}

TopDUContext::Features ParseJob::staticMinimumFeatures(IndexedString url)
{
    QMutexLocker lock(&minimumFeaturesMutex);
    TopDUContext::Features features = (TopDUContext::Features)0;
    
    if(::staticMinimumFeatures.contains(url))
        foreach(const TopDUContext::Features &f, ::staticMinimumFeatures[url])
            features = (TopDUContext::Features)(features | f);
    
    return features;
}

TopDUContext::Features ParseJob::minimumFeatures() const
{
    
    return (TopDUContext::Features)(d->features | staticMinimumFeatures(d->document));
}

void ParseJob::setDuChain(ReferencedTopDUContext duChain)
{
    d->duContext = duChain;
}

ReferencedTopDUContext ParseJob::duChain() const
{
    return d->duContext;
}

void ParseJob::addJob(Job* job)
{
    if (backgroundParser())
        job->assignQueuePolicy(backgroundParser()->dependencyPolicy());

    JobSequence::addJob(job);
}

BackgroundParser* ParseJob::backgroundParser() const
{
    return d->backgroundParser;
}

void ParseJob::setBackgroundParser(BackgroundParser* parser)
{
    if (parser) {
        assignQueuePolicy(parser->dependencyPolicy());

        for (int i = 0; i < jobListLength(); ++i)
            jobAt(i)->assignQueuePolicy(parser->dependencyPolicy());

    } else if (d->backgroundParser) {

        removeQueuePolicy(d->backgroundParser->dependencyPolicy());

        for (int i = 0; i < jobListLength(); ++i)
            jobAt(i)->removeQueuePolicy(d->backgroundParser->dependencyPolicy());
    }

    d->backgroundParser = parser;
}

bool ParseJob::addDependency(ParseJob* dependency, ThreadWeaver::Job* actualDependee)
{
    if (!backgroundParser())
        return false;

    return backgroundParser()->dependencyPolicy()->addDependency(dependency, this, actualDependee);
}

bool ParseJob::abortRequested() const
{
    QMutexLocker lock(d->abortMutex);

    return d->abortRequested;
}

void ParseJob::requestAbort()
{
    QMutexLocker lock(d->abortMutex);

    d->abortRequested = true;
}

void ParseJob::abortJob()
{
    d->aborted = true;
    setFinished(true);
}

void ParseJob::setNotifyWhenReady(QList<QPointer<QObject> > notify) {
    d->notify = notify;
}

void ParseJob::setStaticMinimumFeatures(IndexedString url, TopDUContext::Features features) {
    QMutexLocker lock(&minimumFeaturesMutex);
    ::staticMinimumFeatures[url].append(features);
}

void ParseJob::unsetStaticMinimumFeatures(IndexedString url, TopDUContext::Features features) {
    QMutexLocker lock(&minimumFeaturesMutex);
    ::staticMinimumFeatures[url].removeOne(features);
    if(::staticMinimumFeatures[url].isEmpty())
      ::staticMinimumFeatures.remove(url);
}

KDevelop::ProblemPointer ParseJob::readContents()
{
    Q_ASSERT(!d->hasReadContents);
    d->hasReadContents = true;
    
    QString localFile(document().toUrl().toLocalFile());
    QFileInfo fileInfo( localFile );

    QDateTime lastModified = fileInfo.lastModified();
    
    ForegroundLock lock;
    
    //Try using an artificial code-representation, which overrides everything else
    if(artificialCodeRepresentationExists(document())) {
        CodeRepresentation::Ptr repr = createCodeRepresentation(document());
        d->contents.contents = repr->text().toUtf8();
        kDebug() << "took contents for " << document().str() << " from artificial code-representation";
        return KDevelop::ProblemPointer();
    }
    
    if(d->tracker)
    {
        // The file is open in an editor
        d->previousDocumentRevision = d->tracker->revisionAtLastReset();

        if(d->previousDocumentRevision != -1)
            d->previousRevision = d->tracker->acquireRevision(d->previousDocumentRevision);
        
        d->tracker->reset(); // Reset the tracker to the current revision
        
        d->contents.contents = tracker()->textAtLastReset().toUtf8();
        d->contents.modification = KDevelop::ModificationRevision( lastModified, tracker()->revisionAtLastReset() );
        
        d->revision = d->tracker->acquireRevision(d->contents.modification.revision);
    }else{
        // We have to load the file from disk
        
        lock.unlock(); // Unlock the foreground lock before reading from disk, so the UI won't block due to I/O
        
        QFile file( localFile );
        
        if ( !file.open( QIODevice::ReadOnly ) )
        {
            KDevelop::ProblemPointer p(new Problem());
            p->setSource(KDevelop::ProblemData::Disk);
            p->setDescription(i18n( "Could not open file '%1'", localFile ));
            switch (file.error()) {
              case QFile::ReadError:
                  p->setExplanation(i18n("File could not be read from disk."));
                  break;
              case QFile::OpenError:
                  p->setExplanation(i18n("File could not be opened."));
                  break;
              case QFile::PermissionsError:
                  p->setExplanation(i18n("File could not be read from disk due to permissions."));
                  break;
              default:
                  break;
            }
            p->setFinalLocation(DocumentRange(document(), SimpleRange::invalid()));
            
            kWarning( 9007 ) << "Could not open file" << document().str() << "(path" << localFile << ")" ;
            
            return p;
        }
        
        d->contents.contents = file.readAll(); ///@todo Convert from local encoding to utf-8 if they don't match
        d->contents.modification = KDevelop::ModificationRevision(lastModified);
        
        file.close();
    }

    d->contents.contents.push_back((char)0);
    d->contents.contents.push_back((char)0);
    d->contents.contents.push_back((char)0);
    d->contents.contents.push_back((char)0);
    
    return KDevelop::ProblemPointer();
}

const KDevelop::ParseJob::Contents& ParseJob::contents() const
{
    Q_ASSERT(d->hasReadContents);
    return d->contents;
}

struct MovingRangeTranslator : public DUChainVisitor
{
    MovingRangeTranslator(qint64 _source, qint64 _target, MovingInterface* _moving) : source(_source), target(_target), moving(_moving) {
    }
    
    virtual void visit(DUContext* context) {
        translateRange(context);
        ///@todo Also map import-positions
        // Translate uses
        uint usesCount = context->usesCount();
        for(uint u = 0; u < usesCount; ++u)
        {
            RangeInRevision r = context->uses()[u].m_range;
            translateRange(r);
            context->changeUseRange(u, r);
        }
    }
    
    virtual void visit(Declaration* declaration) {
        translateRange(declaration);
    }
    
    void translateRange(DUChainBase* object)
    {
        RangeInRevision r = object->range();
        translateRange(r);
        object->setRange(r);
    }

    void translateRange(RangeInRevision& r)
    {
        moving->transformCursor(r.start.line, r.start.column, MovingCursor::MoveOnInsert, source, target);
        moving->transformCursor(r.end.line, r.end.column, MovingCursor::StayOnInsert, source, target);
    }

    KTextEditor::Range range;
    qint64 source;
    qint64 target;
    MovingInterface* moving;
};

void ParseJob::translateDUChainToRevision(TopDUContext* context)
{
    qint64 targetRevision = d->contents.modification.revision;
    
    if(targetRevision == -1)
        return;
    
    qint64 sourceRevision;

    {
        DUChainReadLocker duChainLock;
        
        Q_ASSERT(context->parsingEnvironmentFile());
        
        // Cannot map if there is no source revision
        sourceRevision = context->parsingEnvironmentFile()->modificationRevision().revision;
        
        if(sourceRevision == -1)
            return;
    }
    
    if(sourceRevision > targetRevision)
    {
        kDebug() << "for document" << document().str() << ": source revision is higher than target revision:" << sourceRevision << " > " << targetRevision;
        return;
    }
    
    ForegroundLock lock;
    if(d->tracker)
    {
        if(sourceRevision != d->previousDocumentRevision)
        {
            kDebug() << "not translating because the document revision does not match the tracker start revision (maybe the document was cleared)";
            return;
        }
        
        if(!d->tracker->holdingRevision(sourceRevision) || !d->tracker->holdingRevision(targetRevision))
        {
            kDebug() << "lost one of the translation revisions, not doing the map";
            return;
        }
        
        // Perform translation
        MovingInterface* moving = d->tracker->documentMovingInterface();
        
        DUChainWriteLocker wLock;
        
        MovingRangeTranslator translator(sourceRevision, targetRevision, moving);
        context->visit(translator);

        QList< ProblemPointer > problems = context->problems();
        for(QList< ProblemPointer >::iterator problem = problems.begin(); problem != problems.end(); ++problem)
        {
            RangeInRevision r = (*problem)->range();
            translator.translateRange(r);
            (*problem)->setRange(r);
        }
        
        // Update the modification revision in the meta-data
        ModificationRevision modRev = context->parsingEnvironmentFile()->modificationRevision();
        modRev.revision = targetRevision;
        context->parsingEnvironmentFile()->setModificationRevision(modRev);
    }
}

}

#include "parsejob.moc"

