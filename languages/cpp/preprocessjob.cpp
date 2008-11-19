/*
* This file is part of KDevelop
*
* Copyright 2006 Adam Treat <treat@kde.org>
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

#include "preprocessjob.h"

//#include <valgrind/memcheck.h>


#include <QFile>
#include <QFileInfo>
#include <QByteArray>
#include <QMutexLocker>
#include <QReadWriteLock>

#include <kdebug.h>
#include <klocale.h>

#include <language/backgroundparser/backgroundparser.h>
#include <language/duchain/duchain.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/topducontext.h>
#include <language/editor/editorintegrator.h>
#include <language/interfaces/iproblem.h>

#include <threadweaver/Thread.h>

#include <interfaces/ilanguage.h>

#include "cpplanguagesupport.h"
#include "cppparsejob.h"
#include "parser/ast.h"
#include "parser/parsesession.h"
#include "parser/rpp/pp-engine.h"
#include "parser/rpp/pp-macro.h"
#include "parser/rpp/preprocessor.h"
#include "environmentmanager.h"
#include "cpppreprocessenvironment.h"

#include "cppdebughelper.h"

// #define ifDebug(x) x

QString urlsToString(const QList<KUrl>& urlList) {
  QString paths;
  foreach( const KUrl& u, urlList )
      paths += u.pathOrUrl() + "\n";

  return paths;
}

PreprocessJob::PreprocessJob(CPPParseJob * parent)
    : ThreadWeaver::Job(parent)
    , m_currentEnvironment(0)
    , m_firstEnvironmentFile( new Cpp::EnvironmentFile( parent->document(), 0 ) )
    , m_success(true)
    , m_headerSectionEnded(false)
    , m_pp(0)
{
}

KDevelop::ParsingEnvironment* PreprocessJob::createStandardEnvironment() {
    CppPreprocessEnvironment* ret = new CppPreprocessEnvironment(0, Cpp::EnvironmentFilePointer());
    ret->merge( CppLanguageSupport::self()->standardMacros() );
    
    return ret;
}

CPPParseJob * PreprocessJob::parentJob() const
{
    return static_cast<CPPParseJob*>(const_cast<QObject*>(parent()));
}

void PreprocessJob::run()
{
    //If we have a parent, that parent already has locked the parse-lock
    QReadLocker lock(parentJob()->parentPreprocessor() ? 0 : parentJob()->cpp()->language()->parseLock());
  
    //It seems like we cannot influence the actual thread priority in thread-weaver, so for now set it here.
    //It must be low so the GUI stays fluid.
    if(QThread::currentThread())
      QThread::currentThread()->setPriority(QThread::LowestPriority);

    //kDebug(9007) << "Started pp job" << this << "parse" << parentJob()->parseJob() << "parent" << parentJob();

    kDebug(9007) << "PreprocessJob: preprocessing" << parentJob()->document().str();

    if (checkAbort())
        return;

    m_firstEnvironmentFile->setIncludePaths( parentJob()->masterJob()->includePaths() );

    {
      KDevelop::DUChainReadLocker readLock(KDevelop::DUChain::lock());
      
    if(Cpp::EnvironmentManager::isSimplifiedMatching()) {
        //Make sure that proxy-contexts and content-contexts never have the same identity, even if they have the same content.
            m_firstEnvironmentFile->setIdentityOffset(1); //Mark the first environment-file as the proxy
            IndexedString u = parentJob()->document();
            m_secondEnvironmentFile = new Cpp::EnvironmentFile(  u, 0 );
            m_secondEnvironmentFile->setIncludePaths(m_firstEnvironmentFile->includePaths());
        }
    }

    rpp::pp preprocessor(this);
    m_pp = &preprocessor;

    //Eventually initialize the environment with the parent-environment to get its macros
    m_currentEnvironment = new CppPreprocessEnvironment( &preprocessor, m_firstEnvironmentFile );

    //If we are included from another preprocessor, copy its macros
    if( parentJob()->parentPreprocessor() ) {
        m_currentEnvironment->swapMacros( parentJob()->parentPreprocessor()->m_currentEnvironment );
    } else {
        //Insert standard-macros
        KDevelop::ParsingEnvironment* standardEnv = createStandardEnvironment();
        m_currentEnvironment->swapMacros( dynamic_cast<CppPreprocessEnvironment*>(standardEnv) );
        delete standardEnv;
    }
    
    KDevelop::ParsingEnvironmentFilePointer updatingEnvironmentFile;
    
    {
        ///Find a context that can be updated, and eventually break processing right here, if we notice we don't need to update
        KDevelop::DUChainReadLocker readLock(KDevelop::DUChain::lock());
        
        updatingEnvironmentFile = KDevelop::ParsingEnvironmentFilePointer( KDevelop::DUChain::self()->environmentFileForDocument(parentJob()->document(), m_currentEnvironment, (bool)m_secondEnvironmentFile) );
        
        if(parentJob()->masterJob() == parentJob() && updatingEnvironmentFile) {
          //Check whether we need to run at all, or whether the file is already up to date
          if(updatingEnvironmentFile->featuresSatisfied(parentJob()->minimumFeatures())) {
            KUrl localPath(parentJob()->document().toUrl());
            localPath.setFileName(QString());
            Cpp::EnvironmentFile* cppEnv = dynamic_cast<Cpp::EnvironmentFile*>(updatingEnvironmentFile.data());
            Q_ASSERT(cppEnv);
            bool needsUpdate = CppLanguageSupport::self()->needsUpdate(Cpp::EnvironmentFilePointer(cppEnv), localPath, parentJob()->includePathUrls());
            
            if(!needsUpdate) {
              parentJob()->setNeedsUpdate(false);
              return;
            }
          }
        }
    }
    
    
    bool readFromDisk = !parentJob()->contentsAvailableFromEditor();
    parentJob()->setReadFromDisk(readFromDisk);

    QByteArray contents;

    QString localFile(parentJob()->document().toUrl().toLocalFile());
  
    QFileInfo fileInfo( localFile );
    
    if ( readFromDisk )
    {
        QFile file( localFile );
        if ( !file.open( QIODevice::ReadOnly ) )
        {
            KDevelop::ProblemPointer p(new Problem());
            p->setSource(KDevelop::Problem::Disk);
            p->setDescription(i18n( "Could not open file '%1'", localFile ));
            switch (file.error()) {
              case QFile::ReadError:
                  p->setExplanation(i18n("File could not be read from."));
                  break;
              case QFile::OpenError:
                  p->setExplanation(i18n("File could not be opened."));
                  break;
              case QFile::PermissionsError:
                  p->setExplanation(i18n("File permissions prevent opening for read."));
                  break;
              default:
                  break;
            }
            p->setFinalLocation(DocumentRange(parentJob()->document().str(), KTextEditor::Cursor::invalid(), KTextEditor::Cursor::invalid()));
            p->setLocationStack(parentJob()->includeStack());
            parentJob()->addPreprocessorProblem(p);

            kWarning( 9007 ) << "Could not open file" << parentJob()->document().str() << "(path" << localFile << ")" ;
            return ;
        }

        contents = file.readAll(); ///@todo respect local encoding settings. Currently, the file is expected to be utf-8
        
    //        Q_ASSERT( !contents.isEmpty() );
        file.close();
        m_firstEnvironmentFile->setModificationRevision( KDevelop::ModificationRevision(fileInfo.lastModified()) );
    }
    else
    {
        KTextEditor::Range range = KTextEditor::Range::invalid();

        //===--- Incremental Parsing!!! yay :) ---===//
        kDebug() << "We could have just parsed the changed ranges:";
        foreach (KTextEditor::SmartRange* range, parentJob()->changedRanges())
            kDebug() << *range << range->text().join("\n").left(20) << "...";

        contents = parentJob()->contentsFromEditor().toUtf8();
        m_firstEnvironmentFile->setModificationRevision( KDevelop::ModificationRevision( fileInfo.lastModified(), parentJob()->revisionToken() ) );
    }
    
    KDevelop::ModificationRevision::clearModificationCache(parentJob()->document());

    if(m_secondEnvironmentFile) //Copy some information from the environment-file to its content-part
        m_secondEnvironmentFile->setModificationRevision(m_firstEnvironmentFile->modificationRevision());

    ifDebug( kDebug( 9007 ) << "===-- PREPROCESSING --===> "
    << parentJob()->document().str()
    << "<== readFromDisk:" << readFromDisk
    << "size:" << contents.length()
    << endl; )

    if (checkAbort())
        return;

    // Create a new macro block if this is the master preprocess-job
    if( parentJob()->masterJob() == parentJob() )
        parentJob()->parseSession()->macros = new rpp::MacroBlock(0);

    {
        ///Find a context that can be updated
        KDevelop::DUChainReadLocker readLock(KDevelop::DUChain::lock());
        
        KDevelop::ReferencedTopDUContext updating;
        if(updatingEnvironmentFile)
          updating = updatingEnvironmentFile->topContext();

        if(m_secondEnvironmentFile)
          parentJob()->setUpdatingProxyContext( updating ); //The content-context to be updated will be searched later
        else
          parentJob()->setUpdatingContentContext( updating );
      
        if( updating ) {
          //We don't need to change anything, because the EnvironmentFile will be replaced with a new one
          m_updatingEnvironmentFile = KSharedPtr<Cpp::EnvironmentFile>( dynamic_cast<Cpp::EnvironmentFile*>(updating->parsingEnvironmentFile().data()) );
        }
        if( m_secondEnvironmentFile && parentJob()->updatingProxyContext() ) {
            // Must be true, because we explicitly passed the flag to chainForDocument
            Q_ASSERT((parentJob()->updatingProxyContext()->parsingEnvironmentFile()->isProxyContext()));
        }
    }
    
    preprocessor.setEnvironment( m_currentEnvironment );

    preprocessor.environment()->enterBlock(parentJob()->masterJob()->parseSession()->macros);

    PreprocessedContents result = preprocessor.processFile(parentJob()->document().str(), contents);

    m_currentEnvironment->finish();
    
    if(!m_headerSectionEnded) {
      ifDebug( kDebug(9007) << parentJob()->document().str() << ": header-section was not ended"; )
      headerSectionEndedInternal(0);
    }
    
    foreach (KDevelop::ProblemPointer p, preprocessor.problems()) {
      p->setLocationStack(parentJob()->includeStack());
      p->setSource(KDevelop::Problem::Preprocessor);
      parentJob()->addPreprocessorProblem(p);
    }

    parentJob()->parseSession()->setContents( result, m_currentEnvironment->takeLocationTable() );
    parentJob()->parseSession()->setUrl( parentJob()->document() );

    if(m_secondEnvironmentFile)
      parentJob()->setProxyEnvironmentFile( m_firstEnvironmentFile.data() );
    else
      parentJob()->setContentEnvironmentFile( m_firstEnvironmentFile.data() );

    if( m_secondEnvironmentFile ) {
        //kDebug(9008) << parentJob()->document().str() << "Merging content-environment file into header environment-file";
        m_firstEnvironmentFile->merge(*m_secondEnvironmentFile);
        parentJob()->setContentEnvironmentFile(m_secondEnvironmentFile.data());
    }
    
    if( PreprocessJob* parentPreprocessor = parentJob()->parentPreprocessor() ) {
        //If we are included from another preprocessor, give it back the modified macros,
        parentPreprocessor->m_currentEnvironment->swapMacros( m_currentEnvironment );
        //Merge include-file-set, defined macros, used macros, and string-set
        parentPreprocessor->m_currentEnvironment->environmentFile()->merge(*m_firstEnvironmentFile);
    }else{
/*        kDebug(9007) << "Macros:";
        for( rpp::Environment::EnvironmentMap::const_iterator it = m_currentEnvironment->environment().begin(); it != m_currentEnvironment->environment().end(); ++it ) {
            kDebug(9007) << (*it)->name.str() << "                  from: " << (*it)->file << ":" << (*it)->sourceLine;
        }*/
    }
    ifDebug( kDebug(9007) << "PreprocessJob: finished" << parentJob()->document().str(); )

    m_currentEnvironment = 0;
    m_pp = 0;
}

void PreprocessJob::headerSectionEnded(rpp::Stream& stream)
{
  headerSectionEndedInternal(&stream);
}

TopDUContext* contentFromProxy(TopDUContext* ctx) {
    if( ctx->parsingEnvironmentFile() && ctx->parsingEnvironmentFile()->isProxyContext() ) {
        Q_ASSERT(!ctx->importedParentContexts().isEmpty());
        return dynamic_cast<TopDUContext*>(ctx->importedParentContexts().first().context(0));
    }else{
        return ctx;
    }
}


void PreprocessJob::headerSectionEndedInternal(rpp::Stream* stream)
{
    bool closeStream = false;
    m_headerSectionEnded = true;

    ifDebug( kDebug(9007) << parentJob()->document().str() << "PreprocessJob::headerSectionEnded, " << parentJob()->includedFiles().count() << " included in header-section" << "upcoming identity-offset:" << m_pp->branchingHash()*19; )
    
    if( m_secondEnvironmentFile ) {
        m_secondEnvironmentFile->setIdentityOffset(m_pp->branchingHash()*19);

        if( stream ) {
          m_secondEnvironmentFile->setContentStartLine(stream->originalInputPosition().line);
          m_firstEnvironmentFile->setContentStartLine(stream->originalInputPosition().line);
        }

        ///Only allow content-contexts that have the same branching hash,
        ///because else they were differently influenced earlier by macros in the header-section
        ///Example: A file that has completely different content depending on an #ifdef

        m_currentEnvironment->setIdentityOffsetRestriction(m_secondEnvironmentFile->identityOffset());
        
        IndexedString u = parentJob()->document();

        ///Find a matching content-context
        KDevelop::DUChainReadLocker readLock(KDevelop::DUChain::lock());

        KDevelop::ReferencedTopDUContext content;

        if(m_updatingEnvironmentFile)
          content = KDevelop::ReferencedTopDUContext(contentFromProxy(m_updatingEnvironmentFile->topContext()));
        else
          content = KDevelop::DUChain::self()->chainForDocument(u, m_currentEnvironment, false, true);

        m_currentEnvironment->disableIdentityOffsetRestriction();

        if(content) {
            //We have found a content-context that we can use
            parentJob()->setUpdatingContentContext(content);

            Cpp::EnvironmentFilePointer contentEnvironment(dynamic_cast<Cpp::EnvironmentFile*>(content->parsingEnvironmentFile().data()));
            Q_ASSERT(contentEnvironment->identityOffset() == m_secondEnvironmentFile->identityOffset());

            ///@todo think whether localPath is needed
            KUrl localPath(parentJob()->document().str());
            localPath.setFileName(QString());
            
            if(contentEnvironment->matchEnvironment(m_currentEnvironment) && !CppLanguageSupport::self()->needsUpdate(contentEnvironment, localPath, parentJob()->includePathUrls()) && (!parentJob()->masterJob()->needUpdateEverything() || parentJob()->masterJob()->wasUpdated(content)) && (content->parsingEnvironmentFile()->featuresSatisfied(parentJob()->minimumFeatures())) ) {
                //We can completely re-use the specialized context:
                m_secondEnvironmentFile = dynamic_cast<Cpp::EnvironmentFile*>(content->parsingEnvironmentFile().data());
                
                //Merge the macros etc. into the current environment
                m_currentEnvironment->merge( m_secondEnvironmentFile.data() );

                ifDebug( kDebug(9007) << "closing data-stream, body does not need to be processed"; )
                closeStream = true;
                parentJob()->setKeepDuchain(true); //We truncate all following content, so we don't want to update the du-chain.
                Q_ASSERT(m_secondEnvironmentFile);
            } else {
                ifDebug( kDebug(9007) << "updating content-context"; )
                m_updatingEnvironmentFile = KSharedPtr<Cpp::EnvironmentFile>(dynamic_cast<Cpp::EnvironmentFile*>(content->parsingEnvironmentFile().data()));
                //We will re-use the specialized context, but it needs updating. So we keep processing here.
                //We don't need to change m_updatingEnvironmentFile, because we will create a new one.
            }
        } else {
            //We need to process the content ourselves
            ifDebug( kDebug(9007) << "could not find a matching content-context"; )
        }

        m_currentEnvironment->finish();

        m_currentEnvironment->setEnvironmentFile(m_secondEnvironmentFile);
    }

    if( stream ) {
      if( closeStream )
        stream->toEnd();
    }
}

rpp::Stream* PreprocessJob::sourceNeeded(QString& _fileName, IncludeType type, int sourceLine, bool skipCurrentPath)
{
    Q_UNUSED(type)
    
    KUrl fileNameUrl(_fileName);
    
    QString fileName = fileNameUrl.pathOrUrl();
    
    if (checkAbort())
        return 0;

    ifDebug( kDebug(9007) << "PreprocessJob" << parentJob()->document().str() << ": searching for include" << fileName; )

    KUrl localPath(parentJob()->document().str());
    localPath.setFileName(QString());
    QStack<DocumentCursor> includeStack = parentJob()->includeStack();

    KUrl from;
    if (skipCurrentPath)
      from = parentJob()->includedFromPath();

    QPair<KUrl, KUrl> included = parentJob()->cpp()->findInclude( parentJob()->includePathUrls(), localPath, fileName, type, from );
    KUrl includedFile = included.first;
    if (includedFile.isValid()) {
      
        {
          //Prevent recursion that may cause a crash
          IndexedString indexedFile(includedFile);
          PreprocessJob* current = this;
          while(current) {
            if(current->parentJob()->document() == indexedFile) {
              KDevelop::ProblemPointer p(new Problem()); ///@todo create special include-problem
              p->setSource(KDevelop::Problem::Preprocessor);
              p->setDescription(i18n("File was included recursively from within itself: %1", fileName ));
              p->setFinalLocation(DocumentRange(parentJob()->document().str(), KTextEditor::Cursor(sourceLine,0), KTextEditor::Cursor(sourceLine+1,0)));
              p->setLocationStack(parentJob()->includeStack());
              parentJob()->addPreprocessorProblem(p);
              return 0;
            }
            current = current->parentJob()->parentPreprocessor();
          }
        }
      
        if( m_updatingEnvironmentFile && m_updatingEnvironmentFile->missingIncludeFiles().contains(IndexedString(fileName)) ) {
          //We are finding a file that was not in the include-path last time
          //All following contexts need to be updated, because they may contain references to missing declarations
          parentJob()->masterJob()->setNeedUpdateEverything( true );
          kDebug(9007) << "Marking every following encountered context to be updated";
        }

        ifDebug( kDebug(9007) << "PreprocessJob" << parentJob()->document().str() << "(" << m_currentEnvironment->environment().size() << "macros)" << ": found include-file" << fileName << ":" << includedFile; )

        KDevelop::ReferencedTopDUContext includedContext;
        bool updateNeeded = false;

        {
            KDevelop::DUChainReadLocker readLock(KDevelop::DUChain::lock());
            includedContext = KDevelop::DUChain::self()->chainForDocument(includedFile, m_currentEnvironment, (bool)m_secondEnvironmentFile);
            if(includedContext) {
              Cpp::EnvironmentFilePointer includedEnvironment(dynamic_cast<Cpp::EnvironmentFile*>(includedContext->parsingEnvironmentFile().data()));
              if( includedEnvironment )
                updateNeeded = CppLanguageSupport::self()->needsUpdate(includedEnvironment, localPath, parentJob()->includePathUrls()) || !includedEnvironment->featuresSatisfied(parentJob()->minimumFeatures());
            }
        }

        if( includedContext && !updateNeeded && (!parentJob()->masterJob()->needUpdateEverything() || parentJob()->masterJob()->wasUpdated(includedContext)) ) {
            ifDebug( kDebug(9007) << "PreprocessJob" << parentJob()->document().str() << ": took included file from the du-chain" << fileName; )

            KDevelop::DUChainReadLocker readLock(KDevelop::DUChain::lock());
            parentJob()->addIncludedFile(includedContext, sourceLine);
            KDevelop::ParsingEnvironmentFilePointer file = includedContext->parsingEnvironmentFile();
            Cpp::EnvironmentFile* environmentFile = dynamic_cast<Cpp::EnvironmentFile*> (file.data());
            if( environmentFile ) {
                m_currentEnvironment->merge( environmentFile );
                ifDebug( kDebug() << "PreprocessJob" << parentJob()->document().str() << "Merging included file into environment-file"; )
                m_currentEnvironment->environmentFile()->merge( *environmentFile );
            } else {
                ifDebug( kDebug(9007) << "preprocessjob: included file" << includedFile << "found in du-chain, but it has no parse-environment information, or it was not parsed by c++ support"; )
            }
        } else {
            if(updateNeeded)
              kDebug(9007) << "PreprocessJob" << parentJob()->document().str() << ": need to update" << includedFile;
            else if(parentJob()->masterJob()->needUpdateEverything())
              kDebug(9007) << "PreprocessJob" << parentJob()->document().str() << ": needUpateEverything, updating" << includedFile;
            else
              kDebug(9007) << "PreprocessJob" << parentJob()->document().str() << ": no fitting entry for" << includedFile << "in du-chain, parsing";

/*            if( updateNeeded && !parentJob()->masterJob()->needUpdateEverything() ) {
              //When a new include-file was found, that can influence not found declarations in all following encountered contexts, so they all need updating.
              kDebug(9007) << "Marking every following encountered context to be updated";
              parentJob()->masterJob()->setNeedUpdateEverything( true ); //@todo make this a bit more intelligent, instead of updating everything that follows
            }*/
            /// Why bother the threadweaver? We need the preprocessed text NOW so we simply parse the
            /// included file right here. Parallel parsing cannot be used here, because we need the
            /// macros before we can continue.

            // Create a slave-job that will take over our macros.
            // It will itself take our macros modify them, copy them back,
            // and merge information into our m_firstEnvironmentFile

            ///The second parameter is zero because we are in a background-thread and we here
            ///cannot create a slave of the foreground cpp-support-part.
            CPPParseJob* slaveJob = new CPPParseJob(includedFile, 0, this);
            if((parentJob()->minimumFeatures() & TopDUContext::AllDeclarationsContextsAndUsesForRecursive) == TopDUContext::AllDeclarationsContextsAndUsesForRecursive)
              slaveJob->setMinimumFeatures(parentJob()->minimumFeatures());

            slaveJob->setIncludedFromPath(included.second);

            includeStack.append(DocumentCursor(HashedString(parentJob()->document().str()), KTextEditor::Cursor(sourceLine, 0)));
            slaveJob->setIncludeStack(includeStack);

            slaveJob->parseForeground();

            // Add the included file.
            ///@todo Gracefully handle if no duchain is returned, while aborting
            Q_ASSERT(slaveJob->duChain());
            parentJob()->addIncludedFile(slaveJob->duChain(), sourceLine);
            delete slaveJob;
        }
        ifDebug( kDebug(9007) << "PreprocessJob" << parentJob()->document().str() << "(" << m_currentEnvironment->environment().size() << "macros)" << ": file included"; )
    } else {
        kDebug(9007) << "PreprocessJob" << parentJob()->document().str() << ": include not found:" << fileName;
        KDevelop::ProblemPointer p(new Problem()); ///@todo create special include-problem
        p->setSource(KDevelop::Problem::Preprocessor);
        p->setDescription(i18n("Included file was not found: %1", fileName ));
        p->setExplanation(i18n("Searched include path:\n%1", urlsToString(parentJob()->includePathUrls())));
        p->setFinalLocation(DocumentRange(parentJob()->document().str(), KTextEditor::Cursor(sourceLine,0), KTextEditor::Cursor(sourceLine+1,0)));
        p->setLocationStack(parentJob()->includeStack());
        parentJob()->addPreprocessorProblem(p);

        ///@todo respect all the specialties like starting search at a specific path
        ///Before doing that, model findInclude(..) exactly after the standard
        m_firstEnvironmentFile->addMissingIncludeFile(IndexedString(fileName));
    }

    return 0;
}

bool PreprocessJob::checkAbort()
{
  if(!CppLanguageSupport::self()) {
    kDebug(9007) << "Environment-manager disappeared" ;
    return true;
  }
    if (CPPParseJob* parent = parentJob()) {
        if (parent->abortRequested()) {
            parent->abortJob();
            m_success = false;
            setFinished(true);
            return true;
        }

    } else {
        // What... the parent job got deleted??
        kWarning(9007) << "Parent job disappeared!!" ;
        m_success = false;
        setFinished(true);
        return true;
    }

    return false;
}

bool PreprocessJob::success() const
{
    return m_success;
}

#include "preprocessjob.moc"

