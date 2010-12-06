/* This file is part of KDevelop
Copyright 2008 Andreas Pakulat <apaku@gmx.de>
Copyright 2010 David Nolden <david.nolden.kdevelop@art-master.de>

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

#include "sessioncontroller.h"

#include <QtCore/QHash>
#include <QtCore/QDir>
#include <QtCore/QSignalMapper>
#include <QtCore/QStringList>
#include <QtCore/QTimer>

#include <kglobal.h>
#include <kcmdlineargs.h>
#include <kcomponentdata.h>
#include <kconfiggroup.h>
#include <kstandarddirs.h>
#include <klocale.h>
#include <kio/netaccess.h>
#include <kparts/mainwindow.h>
#include <kactioncollection.h>

#include "session.h"
#include "core.h"
#include "uicontroller.h"
#include "sessiondialog.h"
#include "shellextension.h"
#include <interfaces/iprojectcontroller.h>
#include <qapplication.h>
#include <kprocess.h>
#include <sublime/mainwindow.h>
#include <KApplication>
#include <QLineEdit>
#include <KMessageBox>
#include <QGroupBox>
#include <QBoxLayout>
#include <QTimer>
#include <QStandardItemModel>
#include <QTreeView>
#include <QHeaderView>
#include <klockfile.h>
#include <interfaces/idocumentcontroller.h>
#include <ktexteditor/document.h>
#include <sublime/area.h>
#include <QLabel>


#include <kdeversion.h>

#if KDE_IS_VERSION(4,5,60)
    #define HAVE_RECOVERY_INTERFACE
    #include <ktexteditor/recoveryinterface.h>
#endif

const int recoveryStorageInterval = 10; ///@todo Make this configurable

namespace KDevelop
{

///Recursive deletion of a directory, should go into some utility file
static bool removeDirectory(const QDir &aDir)
{
  bool has_err = false;
  if (aDir.exists())//QDir::NoDotAndDotDot
  {
    QFileInfoList entries = aDir.entryInfoList(QDir::NoDotAndDotDot | 
    QDir::Dirs | QDir::Files);
    int count = entries.size();
    for (int idx = 0; ((idx < count) && !has_err); idx++)
    {
      QFileInfo entryInfo = entries[idx];
      QString path = entryInfo.absoluteFilePath();
      if (entryInfo.isDir())
      {
        has_err = !removeDirectory(QDir(path));
      }
      else
      {
        QFile file(path);
        if (!file.remove())
        has_err = true;
      }
    }
    if (!aDir.rmdir(aDir.absolutePath()))
      has_err = true;
  }
  return !has_err;
}

namespace {
    int argc = 0;
    char** argv = 0; 
};

void SessionController::setArguments(int _argc, char** _argv)
{
    argc = _argc;
    argv = _argv;
}

static QStringList standardArguments()
{
    QStringList ret;
    for(int a = 0; a < argc; ++a)
    {
        QString arg = QString::fromLocal8Bit(argv[a]);
        kWarning() << "ARG:" << "" + arg + "";
/*        if(arg.startsWith("--graphicssystem=") || arg.startsWith("--style="))
        {
            ret << arg;
        }else */
        if(arg.startsWith("-graphicssystem") || arg.startsWith("-style"))
        {
            ret << "-" + arg;
            if(a+1 < argc)
                ret << QString::fromLocal8Bit(argv[a+1]);
        }
    }
    
    kWarning() << "ARGUMENTS: " << ret << "from" << argc;
    
    return ret;
}

class SessionControllerPrivate : public QObject
{
    Q_OBJECT
public:
    SessionControllerPrivate( SessionController* s ) : q(s), activeSession(0), recoveryDirectoryIsOwn(false) {
        recoveryTimer.setInterval(recoveryStorageInterval * 1000);
        connect(&recoveryTimer, SIGNAL(timeout()), SLOT(recoveryStorageTimeout()));
        
        // Try the recovery only after the initialization has finished
        connect(ICore::self(), SIGNAL(initializationCompleted()), SLOT(lateInitialization()), Qt::QueuedConnection);
        
        recoveryTimer.setSingleShot(false);
        recoveryTimer.start();
    }

    ~SessionControllerPrivate() {
        clearRecoveryDirectory();
    }

    Session* findSessionForName( const QString& name ) const
    {
        foreach( Session* s, sessionActions.keys() )
        {
            if( s->name() == name )
                return s;
        }
        return 0;
    }
    
    Session* findSessionForId(QString idString)
    {
        QUuid id(idString);
        
        foreach( Session* s, sessionActions.keys() )
        {
            if( s->id() == id)
                return s;
        }
        return 0;
    }
    
    void newSession()
    {
        qsrand(QDateTime::currentDateTime().toTime_t());
        Session* session = new Session( QUuid::createUuid() );
        
        KProcess::startDetached(ShellExtension::getInstance()->binaryPath(), QStringList() << "-s" << session->id().toString() << standardArguments());
        delete session;
#if 0        
        //Terminate this instance of kdevelop if the user agrees
        foreach(Sublime::MainWindow* window, Core::self()->uiController()->controller()->mainWindows())
            window->close();
#endif
    }
    
    void configureSessions()
    {
        SessionDialog dlg(ICore::self()->uiController()-> activeMainWindow());
        dlg.exec();
    }

    void deleteSession()
    {
        int choice = KMessageBox::warningContinueCancel(Core::self()->uiController()->activeMainWindow(), i18n("The current session and all contained settings will be deleted. The projects will stay unaffected. Do you really want to continue?"));
        
        if(choice == KMessageBox::Continue)
        {
            static_cast<Session*>(q->activeSession())->deleteFromDisk();
            q->emitQuitSession();
        }
    }

    void renameSession()
    {
        KDialog dialog;
        dialog.setWindowTitle(i18n("Rename Session"));
        QGroupBox box;
        QHBoxLayout layout(&box);
        
        box.setTitle(i18n("New Session Name"));
        QLineEdit edit;
        layout.addWidget(&edit);
        dialog.setButtons(KDialog::Ok | KDialog::Cancel);
        edit.setText(q->activeSession()->name());
        dialog.setMainWidget(&box);
        
        edit.setFocus();
        
        if(dialog.exec() == QDialog::Accepted)
        {
            static_cast<Session*>(q->activeSession())->setName(edit.text());
        }
    }

    bool loadSessionExternally( Session* s )
    {
        Q_ASSERT( s );
        if( !SessionController::tryLockSession(s->id().toString()) )
        {
            KMessageBox::error(ICore::self()->uiController()->activeMainWindow(), i18n("The selected session is already active in another running instance"));
            return false;
        }else{
            KProcess::startDetached(ShellExtension::getInstance()->binaryPath(), QStringList() << "-s" << s->id().toString() << standardArguments());
            return true;
        }
    }
    
    void activateSession( Session* s )
    {
        Q_ASSERT( s );

        KConfigGroup grp = KGlobal::config()->group( SessionController::cfgSessionGroup() );
        grp.writeEntry( SessionController::cfgActiveSessionEntry(), s->id().toString() );
        grp.sync();
        activeSession = s;
        if (Core::self()->setupFlags() & Core::NoUi) return;

        QHash<Session*,QAction*>::iterator it = sessionActions.find(s);
        Q_ASSERT( it != sessionActions.end() );
        (*it)->setCheckable(true);
        (*it)->setChecked(true);
        
        for(it = sessionActions.begin(); it != sessionActions.end(); ++it)
        {
            if(it.key() != s)
                (*it)->setCheckable(false);
        }
        
    }

    void loadSessionFromAction( QAction* a )
    {
        foreach( Session* s, sessionActions.keys() )
        {
            if( s->id() == QUuid( a->data().toString() ) && s != activeSession ) {
                bool loaded = loadSessionExternally( s );
                break;
            }
        }
    }

    void addSession( Session* s )
    {
        if (Core::self()->setupFlags() & Core::NoUi) {
            sessionActions[s] = 0;
            return;
        }

        KAction* a = new KAction( grp );
        a->setText( s->description() );
        a->setCheckable( false );
        a->setData( s->id().toString() );
        sessionActions[s] = a;
        q->actionCollection()->addAction( "session_"+s->id().toString(), a );
        q->unplugActionList( "available_sessions" );
        q->plugActionList( "available_sessions", grp->actions() );
        connect(s, SIGNAL(nameChanged(QString, QString)), SLOT(nameChanged()));
    }

    SessionController* q;

    QHash<Session*, QAction*> sessionActions;
    ISession* activeSession;
    QActionGroup* grp;
    
    KLockFile::Ptr sessionLock;
    
    // Whether this process owns the recovery directory
    bool recoveryDirectoryIsOwn;
    
    QTimer recoveryTimer;
    QMap<KUrl, QStringList > currentRecoveryFiles;

    
    QString ownSessionDirectory() const
    {
        Q_ASSERT(activeSession);
        return SessionController::sessionDirectory() + "/" + activeSession->id().toString();
    }
    
    void clearRecoveryDirectory()
    {
        QDir recoveryDir(ownSessionDirectory() + "/recovery");
        
        if(recoveryDir.exists())
            removeDirectory(recoveryDir);
    }
    
public slots:
    void documentSaved( KDevelop::IDocument* document )
    {
        if(currentRecoveryFiles.contains(document->url()))
        {
            kDebug() << "deleting recovery-info for" << document->url();
            foreach(const QString& recoveryFileName, currentRecoveryFiles[document->url()])
            {
                bool result = QFile::remove(recoveryFileName);
                kDebug() << "deleted" << recoveryFileName << result;
            }
            currentRecoveryFiles.remove(document->url());
        }
    }
    
private slots:
    void lateInitialization()
    {
        performRecovery();
        connect(Core::self()->documentController(), SIGNAL(documentSaved(KDevelop::IDocument*)), SLOT(documentSaved(KDevelop::IDocument*)));
        
    }
    void performRecovery()
    {
        kDebug() << "Checking recovery";
        QDir recoveryDir(ownSessionDirectory() + "/recovery");
        
        if(recoveryDir.exists())
        {
            kDebug() << "Have recovery directory, starting recovery";
            QFile dateFile(recoveryDir.path() + "/date");
            dateFile.open(QIODevice::ReadOnly);
            QString date = QString::fromUtf8(dateFile.readAll());
            
            QDir recoverySubDir(recoveryDir.path() + "/current");
            
            if(!recoverySubDir.exists())
                recoverySubDir = QDir(recoveryDir.path() + "/backup");
            
            if(recoverySubDir.exists())
            {
                kWarning() << "Starting recovery from " << recoverySubDir.absolutePath();
                
                QStringList urlList;
                
                for(uint num = 0; ; ++num)
                {
                    QFile urlFile(recoverySubDir.path() + QString("/%1_url").arg(num));
                    if(!urlFile.exists())
                        break;
                    urlFile.open(QIODevice::ReadOnly);
                    KUrl originalFile(QString::fromUtf8(urlFile.readAll()));
                    urlList << originalFile.pathOrUrl();
                }
                
                if(!urlList.isEmpty())
                {
                    //Either recover, or delete the recovery directory
                    ///TODO: user proper runtime locale for date, it might be different
                    ///      from what was used when the recovery file was saved
                    KGuiItem recover = KStandardGuiItem::cont();
                    recover.setIcon(KIcon("edit-redo"));
                    recover.setText(i18n("Recover"));
                    KGuiItem discard = KStandardGuiItem::discard();
                    int choice = KMessageBox::warningContinueCancelList(qApp->activeWindow(),
                        i18nc("%1: date of the last snapshot",
                              "The session crashed the last time it was used. "
                              "The following modified files can be recovered from a backup from %1.", date),
                        urlList, i18n("Crash Recovery"), recover, discard );

                    if(choice == KMessageBox::Continue)
                    {
                        #if 0
                        {
                            //Put the recovered documents into the "Review" area, and clear the working set
                            ICore::self()->uiController()->switchToArea("review", KDevelop::IUiController::ThisWindow);
                            Sublime::MainWindow* window = static_cast<Sublime::MainWindow*>(ICore::self()->uiController()->activeMainWindow());
                            window->area()->setWorkingSet("recover");
                            window->area()->clearViews();
                        }
                        #endif
                        
                        //Recover the files
                        
                        for(uint num = 0; ; ++num)
                        {
                            QFile urlFile(recoverySubDir.path() + QString("/%1_url").arg(num));
                            if(!urlFile.exists())
                                break;
                            urlFile.open(QIODevice::ReadOnly);
                            KUrl originalFile(QString::fromUtf8(urlFile.readAll()));
                            
                            QFile f(recoverySubDir.path() + "/" + QString("/%1_text").arg(num));
                            f.open(QIODevice::ReadOnly);
                            QString text = QString::fromUtf8(f.readAll());
                            
                            if(text.isEmpty())
                            {
                                KMessageBox::error(ICore::self()->uiController()->activeMainWindow(), i18n("Could not recover %1, the recovery file is empty", originalFile.pathOrUrl()), i18n("Recovery"));
                                continue;
                            }
                            
                            kDebug() << "opening" << originalFile << "for recovery";
                            KDevelop::IDocument* doc = ICore::self()->documentController()->openDocument(originalFile);
                            if(!doc || !doc->textDocument())
                            {
                                kWarning() << "The document " << originalFile.prettyUrl() << " could not be opened as a text-document, creating a new document with the recovered contents";
                                doc = ICore::self()->documentController()->openDocumentFromText(text);
                            }else{
                                #ifdef HAVE_RECOVERY_INTERFACE
                                KTextEditor::RecoveryInterface* recovery = qobject_cast<KTextEditor::RecoveryInterface*>(doc->textDocument());
                                
                                if(recovery && recovery->isDataRecoveryAvailable())
                                    // Use the recovery from the kate swap-file if possible
                                    recovery->recoverData();
                                else
                                    // Use a simple recovery through "replace text"
                                    doc->textDocument()->setText(text);
                                #else
                                    // Use a simple recovery through "replace text"
                                    doc->textDocument()->setText(text);
                                #endif
                            }
                        }
                    }
                }
            }
        }
        
        recoveryDirectoryIsOwn = true;
    }
    
    void nameChanged()
    {
        Q_ASSERT(qobject_cast<Session*>(sender()));
        Session* s = static_cast<Session*>(sender());
        sessionActions[s]->setText( s->description() );
    }
    
    void recoveryStorageTimeout()
    {
        if(!recoveryDirectoryIsOwn)
            return;
        
        currentRecoveryFiles.clear();
        
        QDir recoveryDir(ownSessionDirectory() + "/recovery");
        
        if(!recoveryDir.exists())
        {
            // Try "taking" the recovery directory
            QDir sessionDir(ownSessionDirectory());
            if(!sessionDir.mkdir("recovery"))
                return;
        }

        recoveryDir.mkpath("backup"); //Just to make sure that the recovery directory actually exists
        {
            //Clear the backup dir
            QDir recoveryBackupDir(recoveryDir.path() + "/backup");
            foreach(const QFileInfo& file, recoveryBackupDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Files | QDir::Dirs))
            {
                QFile::remove(file.absoluteFilePath());
            }
            
            bool removed = recoveryDir.rmdir("backup");
            Q_ASSERT(removed);
        }
        
        //Make the current recovery dir the backup dir, so we always have a recovery available
        recoveryDir.rename("current", "backup");

        {
            recoveryDir.mkdir("current_incomplete");
            
            QDir recoveryCurrentDir(recoveryDir.path() + "/current_incomplete");

            uint num = 0;
            
            foreach(KDevelop::IDocument* document, ICore::self()->documentController()->openDocuments())
            {
                if(document->state() == IDocument::Modified || document->state() == IDocument::DirtyAndModified)
                {
                    //This document was modified, create a recovery-backup
                    if(document->textDocument())
                    {
                        //Currently we can only back-up text documents
                        QString text = document->textDocument()->text();
                        
                        if(!text.isEmpty())
                        {
                            QString urlFilePath = recoveryCurrentDir.path() + QString("/%1_url").arg(num);
                            QFile urlFile(urlFilePath);
                            urlFile.open(QIODevice::WriteOnly);
                            urlFile.write(document->url().pathOrUrl().toUtf8());
                            
                            QString textFilePath = recoveryCurrentDir.path() + "/" + QString("/%1_text").arg(num);
                            QFile f(textFilePath);
                            f.open(QIODevice::WriteOnly);
                            f.write(text.toUtf8());
                            
                            currentRecoveryFiles[document->url()] =
                                        QStringList() <<  (recoveryDir.path() + "/current" + QString("/%1_url").arg(num))
                                                      << (recoveryDir.path() + "/current" + QString("/%1_text").arg(num));
                            
                            ++num;
                        }
                    }
                }
            }
        }
        
        recoveryDir.rename("current_incomplete", "current");
        
        {
            //Write down the date of the recovery
            QFile dateFile(recoveryDir.path() + "/date");
            dateFile.open(QIODevice::WriteOnly);
            dateFile.write(QDateTime::currentDateTime().toString(Qt::DefaultLocaleShortDate).toUtf8());
        }
    }
};

bool SessionController::lockSession()
{
    d->sessionLock = new KLockFile(d->ownSessionDirectory() + "/lock");
    KLockFile::LockResult result = d->sessionLock->lock(KLockFile::NoBlockFlag | KLockFile::ForceFlag);
    
    return result == KLockFile::LockOK;
}

void SessionController::updateSessionDescriptions()
{
    for(QHash< Session*, QAction* >::iterator it = d->sessionActions.begin(); it != d->sessionActions.end(); ++it) {
        it.key()->updateDescription();
        if (*it) (*it)->setText(it.key()->description());
    }
}

SessionController::SessionController( QObject *parent )
        : QObject( parent ), d(new SessionControllerPrivate(this))
{
    setObjectName("SessionController");
    setComponentData(KComponentData("kdevsession"));
    
    setXMLFile("kdevsessionui.rc");

    if (Core::self()->setupFlags() & Core::NoUi) return;

    KAction* action = actionCollection()->addAction( "new_session", this, SLOT( newSession() ) );
    action->setText( i18n("Start New Session") );
    action->setToolTip( i18n("Start a new KDevelop instance with an empty session") );
    action->setIcon(KIcon("window-new"));

    action = actionCollection()->addAction( "rename_session", this, SLOT( renameSession() ) );
    action->setText( i18n("Rename Session...") );
    action->setIcon(KIcon("edit-rename"));

    action = actionCollection()->addAction( "delete_session", this, SLOT( deleteSession() ) );
    action->setText( i18n("Delete Session...") );
    action->setIcon(KIcon("edit-delete"));

    action = actionCollection()->addAction( "quit", this, SIGNAL( quitSession() ) );
    action->setText( i18n("Quit") );
    action->setShortcut(Qt::CTRL | Qt::Key_Q);
    action->setIcon(KIcon("application-exit"));

    #if 0
    action = actionCollection()->addAction( "configure_sessions", this, SLOT( configureSessions() ) );
    action->setText( i18n("Configure Sessions...") );
    action->setToolTip( i18n("Create/Delete/Activate Sessions") );
    action->setWhatsThis( i18n( "<b>Configure Sessions</b><p>Shows a dialog to Create/Delete Sessions and set a new active session.</p>" ) );
    #endif

    d->grp = new QActionGroup( this );
    connect( d->grp, SIGNAL(triggered(QAction*)), this, SLOT(loadSessionFromAction(QAction*)) );
}

SessionController::~SessionController()
{
    delete d;
}

void SessionController::startNewSession()
{
    d->newSession();
}

void SessionController::cleanup()
{
    qDeleteAll(d->sessionActions);
}

void SessionController::initialize( const QString& session )
{
    QDir sessiondir( SessionController::sessionDirectory() );
    
    foreach( const QString& s, sessiondir.entryList( QDir::AllDirs ) )
    {
        QUuid id( s );
        if( id.isNull() )
            continue;
        // Only create sessions for directories that represent proper uuid's
        Session* ses = new Session( id );

        //Delete sessions that have no name and are empty
        if( ses->containedProjects().isEmpty() && ses->name().isEmpty()
            && (session.isEmpty() || (ses->id().toString() != session && ses->name() != session)) )
        {
            ///@todo Think about when we can do this. Another instance might still be using this session.
//             session->deleteFromDisk();
            delete ses;
        }else{
            d->addSession( ses );
        }
    }
    loadDefaultSession( session );

    connect(Core::self()->projectController(), SIGNAL(projectClosed(KDevelop::IProject*)), SLOT(updateSessionDescriptions()));
    connect(Core::self()->projectController(), SIGNAL(projectOpened(KDevelop::IProject*)), SLOT(updateSessionDescriptions()));
    updateSessionDescriptions();
}


ISession* SessionController::activeSession() const
{
    return d->activeSession;
}

void SessionController::loadSession( const QString& nameOrId )
{
    d->loadSessionExternally( session( nameOrId ) );
}

QList<QString> SessionController::sessionNames() const
{
    QStringList l;
    foreach( const Session* s, d->sessionActions.keys() )
    {
        l << s->name();
    }
    return l;
}

QList< const KDevelop::Session* > SessionController::sessions() const
{
    QList< const KDevelop::Session* > ret;
    foreach( const Session* s, d->sessionActions.keys() )
    {
        ret << s;
    }
    return ret;
}

Session* SessionController::createSession( const QString& name )
{
    Session* s;
    if(name.startsWith("{"))
    {
        s = new Session( QUuid(name) );
    }else{
        qsrand(QDateTime::currentDateTime().toTime_t());
        s = new Session( QUuid::createUuid() );
        s->setName( name );
    }
    d->addSession( s );
    return s;
}

void SessionController::deleteSession( const QString& nameOrId )
{
    Session* s  = session(nameOrId);
    
    Q_ASSERT( s != d->activeSession ) ;
    
    QHash<Session*,QAction*>::iterator it = d->sessionActions.find(s);
    Q_ASSERT( it != d->sessionActions.end() );

    unplugActionList( "available_sessions" );
    d->grp->removeAction(*it);
    actionCollection()->removeAction(*it);
    plugActionList( "available_sessions", d->grp->actions() );
    s->deleteFromDisk();
    emit sessionDeleted( s->name() );
    d->sessionActions.remove(s);
    s->deleteLater();
}

void SessionController::loadDefaultSession( const QString& session )
{
    QString load = session;
    if (load.isEmpty()) {
        KConfigGroup grp = KGlobal::config()->group( cfgSessionGroup() );
        load = grp.readEntry( cfgActiveSessionEntry(), "default" );
    }

    Session* s = this->session( load );
    if( !s )
    {
        s = createSession( load );
    }
    d->activateSession( s );
}

Session* SessionController::session( const QString& nameOrId ) const
{
    Session* ret = d->findSessionForName( nameOrId );
    if(ret)
        return ret;

    return d->findSessionForId( nameOrId );
}

QString SessionController::cloneSession( const QString& nameOrid )
{
    Session* origSession = session( nameOrid );
    qsrand(QDateTime::currentDateTime().toTime_t());
    QUuid id = QUuid::createUuid();
    KIO::NetAccess::dircopy( KUrl( sessionDirectory() + '/' + origSession->id().toString() ), 
                             KUrl( sessionDirectory() + '/' + id.toString() ), 
                             Core::self()->uiController()->activeMainWindow() );
    Session* newSession = new Session( id );
    newSession->setName( i18n( "Copy of %1", origSession->name() ) );
    d->addSession(newSession);
    return newSession->name();
}

void SessionController::plugActions()
{
    unplugActionList( "available_sessions" );
    plugActionList( "available_sessions", d->grp->actions() );
}


QString SessionController::cfgSessionGroup() { return "Sessions"; }
QString SessionController::cfgActiveSessionEntry() { return "Active Session ID"; }

QList< SessionInfo > SessionController::availableSessionInfo()
{
    QList< SessionInfo > available;

    QDir sessiondir( SessionController::sessionDirectory() );
    foreach( const QString& s, sessiondir.entryList( QDir::AllDirs ) )
    {
        QUuid id( s );
        if( id.isNull() )
            continue;
        // TODO: Refactor the code here and in session.cpp so its shared
        SessionInfo si;
        si.uuid = id;
        KSharedConfig::Ptr config = KSharedConfig::openConfig( sessiondir.absolutePath() + "/" + s +"/sessionrc" );

        QString desc = config->group( "" ).readEntry( "SessionName", "" );
        si.name = desc;

        si.projects = config->group( "General Options" ).readEntry( "Open Projects", QStringList() );

        QString prettyContents = config->group("").readEntry( "SessionPrettyContents", "" );

        if(!prettyContents.isEmpty())
        {
            if(!desc.isEmpty())
                desc += ":  ";
            desc += prettyContents;
        }
        si.description = desc;
        available << si;
    }
    return available;
}

QString SessionController::sessionDirectory()
{
    return KGlobal::mainComponent().dirs()->saveLocation( "data", KGlobal::mainComponent().componentName()+"/sessions", true );
}

SessionController::LockSessionState SessionController::tryLockSession(QString id)
{
    LockSessionState ret;
    
    ret.lockFile = sessionDirectory() + "/" + id + "/lock";

    if(!QFileInfo(ret.lockFile).exists())
    {
        // Maybe the session doesn't exist yet
        ret.success = true;
        return ret;
    }
    
    KLockFile::Ptr lock(new KLockFile(ret.lockFile));
    ret.success = lock->lock(KLockFile::NoBlockFlag | KLockFile::ForceFlag) == KLockFile::LockOK;
    if(!ret.success) {
        lock->getLockInfo(ret.holderPid, ret.holderHostname, ret.holderApp);
    }
    return ret;
}

// Internal helper class
class SessionChooserDialog : public KDialog {
    Q_OBJECT
public:
    SessionChooserDialog(QTreeView* view, QStandardItemModel* model);
    
public Q_SLOTS:
    void updateState();
    void doubleClicked(QModelIndex);
private:
    QStandardItemModel* m_model;
    QTimer m_updateStateTimer;
};

SessionChooserDialog::SessionChooserDialog(QTreeView* view, QStandardItemModel* model) : m_model(model) {
    m_updateStateTimer.setInterval(5000);
    m_updateStateTimer.setSingleShot(false);
    m_updateStateTimer.start();
    connect(&m_updateStateTimer, SIGNAL(timeout()), SLOT(updateState()));
    connect(view, SIGNAL(doubleClicked(QModelIndex)), SLOT(doubleClicked(QModelIndex)));
}

void SessionChooserDialog::doubleClicked(QModelIndex index)
{
    QStandardItem* item = m_model->itemFromIndex(index);
    if(item && item->isEnabled())
        accept();
}

void SessionChooserDialog::updateState() {
    // Sometimes locking may take some time, so we stop the timer, to prevent an 'avalanche' of events
    m_updateStateTimer.stop();
    for(int row = 0; row < m_model->rowCount(); ++row)
    {
        QString session = m_model->index(row, 0).data().toString();
        
        if(session == i18n("Create New Session"))
            continue;
        
        bool running = false;
        QString state;
        SessionController::LockSessionState lockState = KDevelop::SessionController::tryLockSession(session);
        if(!lockState)
        {
            state = i18n("[running, pid %1, app %2, host %3]", lockState.holderPid,
                         lockState.holderApp, lockState.holderHostname);
            running = true;
        }
        
        if(m_model->item(row, 2))
            m_model->item(row, 2)->setText(state);
        
        for(int col = 0; col < 3; ++col)
            if(m_model->item(row, col))
                m_model->item(row, col)->setEnabled(!running);
    }
    
    m_updateStateTimer.start();
}

QString SessionController::showSessionChooserDialog(QString headerText)
{
    QTreeView* view = new QTreeView;
    QStandardItemModel* model = new QStandardItemModel(view);
    SessionChooserDialog dialog(view, model);
    view->setRootIsDecorated(false);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QVBoxLayout layout(dialog.mainWidget());
    if(!headerText.isEmpty())
        layout.addWidget(new QLabel(headerText));
    
    model->setColumnCount(3);
    model->setHeaderData(0, Qt::Horizontal,i18n("Identity"));
    model->setHeaderData(1, Qt::Horizontal, i18n("Contents"));
    model->setHeaderData(2, Qt::Horizontal,i18n("State"));
    view->setModel(model);
    layout.addWidget(view);
    
    int row = 0;
    int defaultRow = 0;
    
    QString defaultSession = KGlobal::config()->group( cfgSessionGroup() ).readEntry( cfgActiveSessionEntry(), "default" );
    
    
    foreach(const KDevelop::SessionInfo& si, KDevelop::SessionController::availableSessionInfo())
    {
        if ( si.name.isEmpty() && si.projects.isEmpty() ) {
            continue;
        }
        
        if(si.uuid.toString() == defaultSession)
            defaultRow = row;
        
        model->setItem(row, 0, new QStandardItem(si.uuid.toString()));
        model->setItem(row, 1, new QStandardItem(si.description));
        
        QString state;
        bool running = false;
        if(!KDevelop::SessionController::tryLockSession(si.uuid.toString()))
            running = true;
        
        model->setItem(row, 2, new QStandardItem(""));
        
        if(defaultRow == row && running)
            ++defaultRow;
        
        ++row;
    }
    
    model->setItem(row, 0, new QStandardItem(i18n("Create New Session")));

    dialog.updateState();
    
    view->selectionModel()->setCurrentIndex(model->index(defaultRow, 0), QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
    
    view->resizeColumnToContents(0);
    view->resizeColumnToContents(1);
    view->resizeColumnToContents(2);
    view->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
    ///@todo We need a way to get a proper size-hint from the view, but unfortunately, that only seems possible after the view was shown.
    dialog.setInitialSize(QSize(900, 600));

    if(dialog.exec() != QDialog::Accepted)
    {
        return QString();
    }
    
    QModelIndex selected = view->selectionModel()->currentIndex();
    if(selected.isValid())
    {
        selected = selected.sibling(selected.row(), 0);
        QString ret = selected.data().toString();
        if(ret == i18n("Create New Session"))
        {
            qsrand(QDateTime::currentDateTime().toTime_t());
            ret = QUuid::createUuid().toString();
        }
        return ret;
    }
    
    return QString();
}

}
#include "sessioncontroller.moc"
#include "moc_sessioncontroller.cpp"
