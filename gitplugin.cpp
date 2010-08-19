/***************************************************************************
 *   Copyright 2008 Evgeniy Ivanov <powerfox@kde.ru>                       *
 *   Copyright 2009 Hugo Parente Lima <hugo.pl@gmail.com>                  *
 *   Copyright 2010 Aleix Pol Gonzalez <aleixpol@kde.org>                  *
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

#include "gitplugin.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KLocalizedString>
#include <KAboutData>
#include <KDebug>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>

#include <interfaces/icore.h>
#include <interfaces/iprojectcontroller.h>
#include <interfaces/iproject.h>

#include <vcs/vcsjob.h>
#include <vcs/vcsrevision.h>
#include <vcs/vcsevent.h>
#include <vcs/dvcs/dvcsjob.h>
#include <vcs/vcsannotation.h>
#include <vcs/widgets/standardvcslocationwidget.h>
#include <KIO/CopyJob>
#include "gitclonejob.h"

K_PLUGIN_FACTORY(KDevGitFactory, registerPlugin<GitPlugin>(); )
K_EXPORT_PLUGIN(KDevGitFactory(KAboutData("kdevgit","kdevgit",ki18n("Git"),"0.1",ki18n("A plugin to support git version control systems"), KAboutData::License_GPL)))

using namespace KDevelop;

namespace
{
    
QDir dotGitDirectory(const KUrl& dirPath)
{
    const QString initialPath(dirPath.toLocalFile());
    const QFileInfo finfo(initialPath);
    QDir dir = finfo.absoluteDir();
    
    static const QString gitDir(".git");
    while (!dir.exists(gitDir) && dir.cdUp()) {} // cdUp, until there is a sub-directory called .git
    
    return dir;
}

/**
 * Whenever a directory is provided, change it for all the files in it but not inner directories,
 * that way we make sure we won't get into recursion,
 */
static KUrl::List preventRecursion(const KUrl::List& urls)
{
    KUrl::List ret;
    foreach(const KUrl& url, urls) {
        QDir d(url.toLocalFile());
        if(d.exists()) {
            QStringList entries = d.entryList(QDir::Files | QDir::NoDotAndDotDot);
            foreach(const QString& entry, entries) {
                KUrl entryUrl = d.absoluteFilePath(entry);
                ret += entryUrl;
            }
        } else
            ret += url;
    }
    return ret;
}

QString toRevisionName(const KDevelop::VcsRevision& rev, QString currentRevision=QString())
{
    switch(rev.revisionType()) {
        case VcsRevision::Special:
            switch(rev.revisionValue().value<VcsRevision::RevisionSpecialType>()) {
                case VcsRevision::Head:
                    return "^HEAD";
                case VcsRevision::Base:
                    return "HEAD";
                case VcsRevision::Working:
                    return "";
                case VcsRevision::Previous:
                    Q_ASSERT(!currentRevision.isEmpty());
                    return currentRevision + "^1";
                case VcsRevision::Start:
                    return "";
                case VcsRevision::UserSpecialType: //Not used
                    Q_ASSERT(false && "i don't know how to do that");
            }
            break;
        case VcsRevision::GlobalNumber:
            return rev.revisionValue().toString();
        case VcsRevision::Date:
        case VcsRevision::FileNumber:
        case VcsRevision::Invalid:
        case VcsRevision::UserSpecialType:
            Q_ASSERT(false);
    }
    return QString();
}

QString revisionInterval(const KDevelop::VcsRevision& rev, const KDevelop::VcsRevision& limit)
{
    QString ret;
    QString srcRevisionName = toRevisionName(rev);
    if(limit.revisionType()==VcsRevision::Special &&
                limit.revisionValue().value<VcsRevision::RevisionSpecialType>()==VcsRevision::Start) //if we want it to the begining just put the revisionInterval
        return ret = srcRevisionName;
    else if(rev.revisionType()==VcsRevision::Special)
        ret = toRevisionName(limit, srcRevisionName);
    else
        ret = toRevisionName(limit, srcRevisionName)+".." +srcRevisionName;
    
    return ret;
}

QDir urlDir(const KUrl& url) { return QFileInfo(url.toLocalFile()).absoluteDir(); }
QDir urlDir(const KUrl::List& urls) { return urlDir(urls.first()); } //TODO: could be improved

}

GitPlugin::GitPlugin( QObject *parent, const QVariantList & )
    : DistributedVersionControlPlugin(parent, KDevGitFactory::componentData())
{
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IBasicVersionControl )
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IDistributedVersionControl )

    core()->uiController()->addToolView(i18n("Git"), dvcsViewFactory());
    setXMLFile("kdevgit.rc");
    setObjectName("Git");
}

GitPlugin::~GitPlugin()
{}

DVcsJob* GitPlugin::errorsFound(const QString& error, KDevelop::OutputJob::OutputJobVerbosity verbosity=OutputJob::Verbose)
{
    DVcsJob* j = new DVcsJob(QDir::temp(), this, verbosity);
    *j << "echo" << i18n("error: %1", error) << "-n";
    return j;
}

void GitPlugin::unload()
{
    core()->uiController()->removeToolView( dvcsViewFactory() );
}


QString GitPlugin::name() const
{
    return QLatin1String("Git");
}

KUrl GitPlugin::repositoryRoot(const KUrl& path)
{
    return KUrl(dotGitDirectory(path).path());
}

bool GitPlugin::isValidDirectory(const KUrl & dirPath)
{
    QDir dir=dotGitDirectory(dirPath);

    return dir.exists(".git");
}

bool GitPlugin::isVersionControlled(const KUrl &path)
{
    QFileInfo fsObject(path.toLocalFile());
    if (fsObject.isDir()) {
        return isValidDirectory(path);
    }

    QString workDir = fsObject.path();
    QString filename = fsObject.fileName();

    QStringList otherFiles = getLsFiles(workDir, QStringList("--") << filename, KDevelop::OutputJob::Silent);
    return !otherFiles.empty();
}

VcsJob* GitPlugin::init(const KUrl &directory)
{
    DVcsJob* job = new DVcsJob(urlDir(directory), this);
    *job << "git" << "init";
    return job;
}

VcsJob* GitPlugin::createWorkingCopy(const KDevelop::VcsLocation & source, const KUrl& dest, KDevelop::IBasicVersionControl::RecursionMode)
{
    DVcsJob* job = new GitCloneJob(urlDir(dest), this);
    *job << "git" << "clone" << "--progress" << "--" << source.localUrl().url() << dest;
    return job;
}

VcsJob* GitPlugin::add(const KUrl::List& localLocations, KDevelop::IBasicVersionControl::RecursionMode recursion)
{
    if (localLocations.empty())
        return errorsFound(i18n("Did not specify the list of files"), OutputJob::Verbose);

    DVcsJob* job = new DVcsJob(urlDir(localLocations), this);
    *job << "git" << "add" << "--" << localLocations;
    return job;
}

KDevelop::VcsJob* GitPlugin::status(const KUrl::List& localLocations, KDevelop::IBasicVersionControl::RecursionMode recursion)
{
    if (localLocations.empty())
        return errorsFound(i18n("Did not specify the list of files"), OutputJob::Verbose);

    DVcsJob* job = new DVcsJob(urlDir(localLocations), this);
    *job << "git" << "status" << "--porcelain" << "--";
    *job << (recursion == IBasicVersionControl::Recursive ? localLocations : preventRecursion(localLocations));

    connect(job, SIGNAL(readyForParsing(KDevelop::DVcsJob*)), SLOT(parseGitStatusOutput(KDevelop::DVcsJob*)));
    return job;
}

VcsJob* GitPlugin::diff(const KUrl& fileOrDirectory, const KDevelop::VcsRevision& srcRevision, const KDevelop::VcsRevision& dstRevision,
                        VcsDiff::Type type, IBasicVersionControl::RecursionMode recursion)
{
    //TODO: control different types
    
    DVcsJob* job = new DVcsJob(urlDir(fileOrDirectory), this);
    *job << "git" << "diff" << "--no-prefix" << revisionInterval(srcRevision, dstRevision) << "--";
    *job << (recursion == IBasicVersionControl::Recursive ? fileOrDirectory : preventRecursion(fileOrDirectory));
    
    connect(job, SIGNAL(readyForParsing(KDevelop::DVcsJob*)), SLOT(parseGitDiffOutput(KDevelop::DVcsJob*)));
    return job;
}

VcsJob* GitPlugin::revert(const KUrl::List& localLocations, IBasicVersionControl::RecursionMode recursion)
{
    if(localLocations.isEmpty() )
        return errorsFound(i18n("Could not revert changes"), OutputJob::Verbose);
    
    DVcsJob* job = new DVcsJob(urlDir(localLocations), this);
    *job << "git" << "checkout" << "--";
    *job << (recursion == IBasicVersionControl::Recursive ? localLocations : preventRecursion(localLocations));
    
    return job;
}


//TODO: git doesn't like empty messages, but "KDevelop didn't provide any message, it may be a bug" looks ugly...
//If no files specified then commit already added files
VcsJob* GitPlugin::commit(const QString& message,
                             const KUrl::List& localLocations,
                             KDevelop::IBasicVersionControl::RecursionMode recursion)
{
    if (localLocations.empty() || message.isEmpty())
        return errorsFound(i18n("No files or message specified"));

    DVcsJob* job = new DVcsJob(dotGitDirectory(localLocations.front()), this);
    
    *job << "git" << "commit" << "-m" << message;
    *job << "--" << localLocations;
    return job;
}

VcsJob* GitPlugin::remove(const KUrl::List& files)
{
    if (files.isEmpty())
        return errorsFound(i18n("No files to remove"));

    DVcsJob* job = new DVcsJob(urlDir(files), this);
    *job << "git" << "rm";
    *job << "--" << files;
    return job;
}

VcsJob* GitPlugin::log(const KUrl& localLocation,
                const KDevelop::VcsRevision& src, const KDevelop::VcsRevision& dst)
{
    DVcsJob* job = new DVcsJob(urlDir(localLocation), this);
    *job << "git" << "log" << revisionInterval(src, dst) << "--date=raw";
    *job << "--" << localLocation;
    connect(job, SIGNAL(readyForParsing(KDevelop::DVcsJob*)), this, SLOT(parseGitLogOutput(KDevelop::DVcsJob*)));
    return job;
}


VcsJob* GitPlugin::log(const KUrl& localLocation,
                const KDevelop::VcsRevision& rev, unsigned long int limit)
{
    DVcsJob* job = new DVcsJob(urlDir(localLocation), this);
    *job << "git" << "log" << "--date=raw" << toRevisionName(rev, QString());
    if(limit>0)
        *job << QString("-%1").arg(limit);
    
    *job << "--" << localLocation;
    connect(job, SIGNAL(readyForParsing(KDevelop::DVcsJob*)), this, SLOT(parseGitLogOutput(KDevelop::DVcsJob*)));
    return job;
}

KDevelop::VcsJob* GitPlugin::annotate(const KUrl &localLocation, const KDevelop::VcsRevision&)
{
    DVcsJob* job = new DVcsJob(urlDir(localLocation), this);
    *job << "git" << "blame" << "--porcelain";
    *job << "--" << localLocation;
    connect(job, SIGNAL(readyForParsing(KDevelop::DVcsJob*)), this, SLOT(parseGitBlameOutput(KDevelop::DVcsJob*)));
    return job;
}

void GitPlugin::parseGitBlameOutput(DVcsJob *job)
{
    QVariantList results;
    VcsAnnotationLine* annotation;
    QStringList lines = job->output().split('\n');
    
    bool skipNext=false;
    QMap<QString, VcsAnnotationLine> definedRevisions;
    for(QStringList::const_iterator it=lines.constBegin(), itEnd=lines.constEnd();
        it!=itEnd; ++it)
    {
        if(skipNext) {
            skipNext=false;
            results += qVariantFromValue(*annotation);
            
            continue;
        }
        
        if(it->isEmpty())
            continue;
        
        QString name = it->left(it->indexOf(' '));
        QString value = it->right(it->size()-name.size()-1);
        
        qDebug() << "last line" << *it;
        if(name=="author")
            annotation->setAuthor(value);
        else if(name=="author-mail") {} //TODO: do smth with the e-mail?
        else if(name=="author-tz") {} //TODO: does it really matter?
        else if(name=="author-time")
            annotation->setDate(QDateTime::fromTime_t(value.toUInt()));
        else if(name=="summary")
            annotation->setCommitMessage(value);
        else if(name.startsWith("committer")) {} //We will just store the authors
        else if(name=="previous") {} //We don't need that either
        else if(name=="filename") { skipNext=true; }
        else if(name=="boundary") {
            definedRevisions.insert("boundary", VcsAnnotationLine());
        }
        else
        {
            QStringList values = value.split(' ');
            
            VcsRevision rev;
            rev.setRevisionValue(name, KDevelop::VcsRevision::GlobalNumber);
            
            skipNext = definedRevisions.contains(name);
            
            if(!skipNext)
                definedRevisions.insert(name, VcsAnnotationLine());
            
            annotation = &definedRevisions[name];
            annotation->setLineNumber(values[1].toInt());
            annotation->setRevision(rev);
        }
    }
    job->setResults(results);
}

DVcsJob* GitPlugin::var(const QString & repository)
{
    DVcsJob* job = new DVcsJob(QDir(repository), this);
    *job << "git" << "var" << "-l";
    return job;
}

DVcsJob* GitPlugin::switchBranch(const QString &repository, const QString &branch)
{
    ///TODO Check if the branch exists. or send only existed branch names here!
    DVcsJob* job = new DVcsJob(QDir(repository), this);
    *job << "git" << "checkout" << branch;
    return job;
}

DVcsJob* GitPlugin::branch(const QString &repository, const QString &basebranch, const QString &branch,
                             const QStringList &args)
{
    DVcsJob* job = new DVcsJob(QDir(repository), this);
    *job << "git" << "branch" << args;
    
    *job << "--";
    if (!branch.isEmpty())
        *job << branch;
    if (!basebranch.isEmpty())
        *job << basebranch;
    return job;
}

VcsJob* GitPlugin::reset(const KUrl& repository, const QStringList &args, const KUrl::List& files)
{
    if(files.isEmpty())
        return errorsFound(i18n("Could not reset"), OutputJob::Verbose);
    
    DVcsJob* job = new DVcsJob(urlDir(repository), this);
    *job << "git" << "reset" << args;
    *job << "--" << files;
    return job;
}

DVcsJob* GitPlugin::lsFiles(const QString &repository, const QStringList &args,
                            OutputJob::OutputJobVerbosity verbosity)
{
    DVcsJob* job = new DVcsJob(QDir(repository), this, verbosity);
    *job << "git" << "ls-files" << args;
    return job;
}

QString GitPlugin::curBranch(const QString &repository)
{
    kDebug() << "Getting branch list";
    
    QScopedPointer<DVcsJob> job(new DVcsJob(QDir(repository), this, OutputJob::Silent));
    *job << "git" << "status" << "-b" << "--short";
    if (job->exec() && job->status() == KDevelop::VcsJob::JobSucceeded) {
        QString out = job->output();
        return out.mid(3, out.indexOf('\n')-3);
    }
    return QString();
}

QStringList GitPlugin::branches(const QString &repository)
{
    QStringList branchListDirty;
    QScopedPointer<DVcsJob> job(branch(repository));
    kDebug() << "Getting branch list";
    
    if (job->exec() && job->status() == KDevelop::VcsJob::JobSucceeded)
        branchListDirty = job->output().split('\n', QString::SkipEmptyParts);
    else
        return QStringList();

    QStringList branchList;
    foreach(QString branch, branchListDirty)
    {
        if (branch.contains('*'))
        {
            branch = branch.prepend('\n').section("\n*", 1);
        }
        else
        {
            branch = branch.prepend('\n').section('\n', 1);
        }
        
        branch = branch.trimmed();
        branchList<<branch;
    }
    return branchList;
}

/* Few words about how this hardcore works:
1. get all commits (with --paretns)
2. select master (root) branch and get all unicial commits for branches (git-rev-list br2 ^master ^br3)
3. parse allCommits. While parsing set mask (columns state for every row) for BRANCH, INITIAL, CROSS,
   MERGE and INITIAL are also set in DVCScommit::setParents (depending on parents count)
   another setType(INITIAL) is used for "bottom/root/first" commits of branches
4. find and set merges, HEADS. It's an ittaration through all commits.
    - first we check if parent is from the same branch, if no then we go through all commits searching parent's index
      and set CROSS/HCROSS for rows (in 3 rows are set EMPTY after commit with parent from another tree met)
    - then we check branchesShas[i][0] to mark heads

4 can be a seporate function. TODO: All this porn require refactoring (rewriting is better)!

It's a very dirty implementation.
FIXME:
1. HEAD which is head has extra line to connect it with further commit
2. If you menrge branch2 to master, only new commits of branch2 will be visible (it's fine, but there will be
extra merge rectangle in master. If there are no extra commits in branch2, but there are another branches, then the place for branch2 will be empty (instead of be used for branch3).
3. Commits that have additional commit-data (not only history merging, but changes to fix conflicts) are shown incorrectly
*/

QList<DVcsEvent> GitPlugin::getAllCommits(const QString &repo)
{
    static bool hasHash = false;
    if (!hasHash)
    {
        initBranchHash(repo);
        hasHash = true;
    }
    QStringList args;
    args << "--all" << "--pretty" << "--parents";
    QScopedPointer<DVcsJob> job(gitRevList(repo, args));
    bool ret = job->exec();
    Q_ASSERT(ret && job->status()==VcsJob::JobSucceeded && "TODO: provide a fall back in case of failing");
    QStringList commits = job->output().split('\n', QString::SkipEmptyParts);

    static QRegExp rx_com("commit \\w{40,40}");

    QList<DVcsEvent>commitList;
    DVcsEvent item;

    //used to keep where we have empty/cross/branch entry
    //true if it's an active branch (then cross or branch) and false if not
    QVector<bool> additionalFlags(branchesShas.count());
    foreach(int flag, additionalFlags)
        flag = false;

    //parse output
    for(int i = 0; i < commits.count(); ++i)
    {
        if (commits[i].contains(rx_com))
        {
            kDebug() << "commit found in " << commits[i];
            item.setCommit(commits[i].section(' ', 1, 1).trimmed());
//             kDebug() << "commit is: " << commits[i].section(' ', 1);

            QStringList parents;
            QString parent = commits[i].section(' ', 2);
            int section = 2;
            while (!parent.isEmpty())
            {
                /*                kDebug() << "Parent is: " << parent;*/
                parents.append(parent.trimmed());
                section++;
                parent = commits[i].section(' ', section);
            }
            item.setParents(parents);

            //Avoid Merge string
            while (!commits[i].contains("Author: "))
                    ++i;

            item.setAuthor(commits[i].section("Author: ", 1).trimmed());
//             kDebug() << "author is: " << commits[i].section("Author: ", 1);

            item.setDate(commits[++i].section("Date:   ", 1).trimmed());
//             kDebug() << "date is: " << commits[i].section("Date:   ", 1);

            QString log;
            i++; //next line!
            while (i < commits.count() && !commits[i].contains(rx_com))
                log += commits[i++];
            --i; //while took commit line
            item.setLog(log.trimmed());
//             kDebug() << "log is: " << log;

            //mask is used in CommitViewDelegate to understand what we should draw for each branch
            QList<int> mask;

            //set mask (properties for each graph column in row)
            for(int i = 0; i < branchesShas.count(); ++i)
            {
                kDebug()<<"commit: " << item.getCommit();
                if (branchesShas[i].contains(item.getCommit()))
                {
                    mask.append(item.getType()); //we set type in setParents

                    //check if parent from the same branch, if not then we have found a root of the branch
                    //and will use empty column for all futher (from top to bottom) revisions
                    //FIXME: we should set CROSS between parent and child (and do it when find merge point)
                    additionalFlags[i] = false;
                    foreach(const QString &sha, item.getParents())
                    {
                        if (branchesShas[i].contains(sha))
                            additionalFlags[i] = true;
                    }
                    if (additionalFlags[i] == false)
                       item.setType(DVcsEvent::INITIAL); //hasn't parents from the same branch, used in drawing
                }
                else
                {
                    if (additionalFlags[i] == false)
                        mask.append(DVcsEvent::EMPTY);
                    else
                        mask.append(DVcsEvent::CROSS);
                }
                kDebug() << "mask " << i << "is " << mask[i];
            }
            item.setProperties(mask);
            commitList.append(item);
        }
    }

    //find and set merges, HEADS, require refactoring!
    for(QList<DVcsEvent>::iterator iter = commitList.begin();
        iter != commitList.end(); ++iter)
    {
        QStringList parents = iter->getParents();
        //we need only only child branches
        if (parents.count() != 1)
            break;

        QString parent = parents[0];
        QString commit = iter->getCommit();
        bool parent_checked = false;
        int heads_checked = 0;

        for(int i = 0; i < branchesShas.count(); ++i)
        {
            //check parent
            if (branchesShas[i].contains(commit))
            {
                if (!branchesShas[i].contains(parent))
                {
                    //parent and child are not in same branch
                    //since it is list, than parent has i+1 index
                    //set CROSS and HCROSS
                    for(QList<DVcsEvent>::iterator f_iter = iter;
                        f_iter != commitList.end(); ++f_iter)
                    {
                        if (parent == f_iter->getCommit())
                        {
                            for(int j = 0; j < i; ++j)
                            {
                                if(branchesShas[j].contains(parent))
                                    f_iter->setPropetry(j, DVcsEvent::MERGE);
                                else
                                    f_iter->setPropetry(j, DVcsEvent::HCROSS);
                            }
                            f_iter->setType(DVcsEvent::MERGE);
                            f_iter->setPropetry(i, DVcsEvent::MERGE_RIGHT);
                            kDebug() << parent << " is parent of " << commit;
                            kDebug() << f_iter->getCommit() << " is merge";
                            parent_checked = true;
                            break;
                        }
                        else
                            f_iter->setPropetry(i, DVcsEvent::CROSS);
                    }
                }
            }
            //mark HEADs

            if (!branchesShas[i].empty() && commit == branchesShas[i][0])
            {
                iter->setType(DVcsEvent::HEAD);
                iter->setPropetry(i, DVcsEvent::HEAD);
                heads_checked++;
                kDebug() << "HEAD found";
            }
            //some optimization
            if (heads_checked == branchesShas.count() && parent_checked)
                break;
        }
    }

    return commitList;
}

void GitPlugin::initBranchHash(const QString &repo)
{
    QStringList branches = GitPlugin::branches(repo);
    kDebug() << "BRANCHES: " << branches;
    //Now root branch is the current branch. In future it should be the longest branch
    //other commitLists are got with git-rev-lits branch ^br1 ^ br2
    QString root = GitPlugin::curBranch(repo);
    QScopedPointer<DVcsJob> job(gitRevList(repo, QStringList(root)));
    bool ret = job->exec();
    Q_ASSERT(ret && job->status()==VcsJob::JobSucceeded && "TODO: provide a fall back in case of failing");
    QStringList commits = job->output().split('\n', QString::SkipEmptyParts);
//     kDebug() << "\n\n\n commits" << commits << "\n\n\n";
    branchesShas.append(commits);
    foreach(const QString &branch, branches)
    {
        if (branch == root)
            continue;
        QStringList args(branch);
        foreach(const QString &branch_arg, branches)
        {
            if (branch_arg != branch)
                //man gitRevList for '^'
                args<<'^' + branch_arg;
        }
        QScopedPointer<DVcsJob> job(gitRevList(repo, args));
        bool ret = job->exec();
        Q_ASSERT(ret && job->status()==VcsJob::JobSucceeded && "TODO: provide a fall back in case of failing");
        QStringList commits = job->output().split('\n', QString::SkipEmptyParts);
//         kDebug() << "\n\n\n commits" << commits << "\n\n\n";
        branchesShas.append(commits);
    }
}

//Actually we can just copy the output without parsing. So it's a kind of draft for future
void GitPlugin::parseLogOutput(const DVcsJob * job, QList<DVcsEvent>& commits) const
{
//     static QRegExp rx_sep( "[-=]+" );
//     static QRegExp rx_date( "date:\\s+([^;]*);\\s+author:\\s+([^;]*).*" );

    static QRegExp rx_com( "commit \\w{1,40}" );

    QStringList lines = job->output().split('\n', QString::SkipEmptyParts);

    DVcsEvent item;
    QString commitLog;

    for (int i=0; i<lines.count(); ++i) {
        QString s = lines[i];
        kDebug() << "line:" << s ;

        if (rx_com.exactMatch(s)) {
            kDebug() << "MATCH COMMIT";
            item.setCommit(s);
            s = lines[++i];
            item.setAuthor(s);
            s = lines[++i];
            item.setDate(s);
            item.setLog(commitLog);
            commits.append(item);
        }
        else
        {
            //FIXME: add this in a loop to the if, like in getAllCommits()
            commitLog += s +'\n';
        }
    }
}

void GitPlugin::parseGitLogOutput(DVcsJob * job)
{
    QList<QVariant> commits;
    static QRegExp commitRegex( "^commit (\\w{8})\\w{32}" );
    static QRegExp infoRegex( "^(\\w+):(.*)" );

    QString contents = job->output();
    QTextStream s(&contents);

    VcsEvent item;
    QString message;
    bool pushCommit = false;
    while (!s.atEnd()) {
        QString line = s.readLine();
        if (commitRegex.exactMatch(line)) {
            if (pushCommit) {
                item.setMessage(message.trimmed());
                commits.append(QVariant::fromValue(item));
            } else {
                pushCommit = true;
            }
            VcsRevision rev;
            rev.setRevisionValue(commitRegex.cap(1), KDevelop::VcsRevision::GlobalNumber);
            item.setRevision(rev);
            message.clear();
        } else if (infoRegex.exactMatch(line)) {
            QString cap1 = infoRegex.cap(1);
            if (cap1 == "Author") {
                item.setAuthor(infoRegex.cap(2).trimmed());
            } else if (cap1 == "Date") {
                item.setDate(QDateTime::fromTime_t(infoRegex.cap(2).trimmed().split(' ')[0].toUInt()));
            }
        } else if (line.startsWith("    ")) {
            message += line.remove(0, 4);
            message += '\n';
        }
    }
    
    item.setMessage(message.trimmed());
    commits.append(QVariant::fromValue(item));
    job->setResults(commits);
}

void GitPlugin::parseGitDiffOutput(DVcsJob* job)
{
    VcsDiff diff;
    diff.setDiff(job->output());
    diff.setBaseDiff(KUrl(job->directory().absolutePath()));
    
    job->setResults(qVariantFromValue(diff));
}

// static VcsStatusInfo::State lsfilesToState(char id)
// {
//     switch(id) {
//         case 'H': return VcsStatusInfo::ItemUpToDate; //Cached
//         case 'S': return VcsStatusInfo::ItemUpToDate; //Skip work tree
//         case 'M': return VcsStatusInfo::ItemHasConflicts; //unmerged
//         case 'R': return VcsStatusInfo::ItemDeleted; //removed/deleted
//         case 'C': return VcsStatusInfo::ItemModified; //modified/changed
//         case 'K': return VcsStatusInfo::ItemDeleted; //to be killed
//         case '?': return VcsStatusInfo::ItemUnknown; //other
//     }
//     Q_ASSERT(false);
//     return VcsStatusInfo::ItemUnknown;
// }

void GitPlugin::parseGitStatusOutput(DVcsJob* job)
{
    QRegExp lineRx(" ?([A-Z?]+) (.+)");
    QStringList outputLines = job->output().split('\n');
    const KUrl workingDir = job->directory().absolutePath();
    
    QVariantList statuses;
    QList<KUrl> processedFiles;
    foreach(const QString& line, outputLines) {
        if(lineRx.indexIn(line)<0) {
            kDebug() << "Couldn't parse git status's output: " << line;
            continue;
        }
        qDebug() << "Checking git status for " << line << lineRx.capturedTexts();
        
        QString curr=lineRx.capturedTexts()[2];
        KUrl fileUrl = workingDir;
        fileUrl.addPath(curr);
        processedFiles.append(fileUrl);
        
        VcsStatusInfo status;
        status.setUrl(fileUrl);
        status.setState(messageToState(lineRx.capturedTexts()[1]));
        
        statuses.append(qVariantFromValue<VcsStatusInfo>(status));
    }
    QStringList paths;
    QStringList oldcmd=job->dvcsCommand();
    QStringList::const_iterator it=oldcmd.constBegin()+oldcmd.indexOf("--")+1, itEnd=oldcmd.constEnd();
    for(; it!=itEnd; ++it)
        paths += *it;
    
    //here we add the already up to date files
    QStringList files = getLsFiles(job->directory().path(), QStringList() << "-c" << "--" << paths, OutputJob::Silent);
    foreach(const QString& file, files) {
        KUrl fileUrl = workingDir;
        fileUrl.addPath(file);
        
        if(!processedFiles.contains(fileUrl)) {
            VcsStatusInfo status;
            status.setUrl(fileUrl);
            status.setState(VcsStatusInfo::ItemUpToDate);
            
            statuses.append(qVariantFromValue<VcsStatusInfo>(status));
        }
    }
    job->setResults(statuses);
}

QStringList GitPlugin::getLsFiles(const QString &directory, const QStringList &args,
    KDevelop::OutputJob::OutputJobVerbosity verbosity)
{
    QScopedPointer<DVcsJob> job(lsFiles(directory, args, verbosity));
    if (job->exec() && job->status() == KDevelop::VcsJob::JobSucceeded)
        return job->output().split('\n', QString::SkipEmptyParts);
    
    return QStringList();
}

DVcsJob* GitPlugin::gitRevParse(const QString &repository, const QStringList &args,
    KDevelop::OutputJob::OutputJobVerbosity verbosity)
{
    DVcsJob* job = new DVcsJob(QDir(repository), this, verbosity);
    *job << "git" << "rev-parse" << args;

    return job;
}

DVcsJob* GitPlugin::gitRevList(const QString &repository, const QStringList &args)
{
    DVcsJob* job = new DVcsJob(QDir(repository), this);
    {
        *job << "git" << "rev-list" << args;
        return job;
    }
}

VcsStatusInfo::State GitPlugin::messageToState(const QString& msg)
{
    Q_ASSERT(msg.size()==1 || msg.size()==2);
    VcsStatusInfo::State ret = VcsStatusInfo::ItemUnknown;
    
    if(msg.contains('U') || msg == "AA" || msg == "DD")
        ret = VcsStatusInfo::ItemHasConflicts;
    else switch(msg[0].toAscii())
    {
        case 'M':
            ret = VcsStatusInfo::ItemModified;
            break;
        case 'A':
            ret = VcsStatusInfo::ItemAdded;
            break;
        case 'D':
            ret = VcsStatusInfo::ItemDeleted;
            break;
        case '?':
            ret = VcsStatusInfo::ItemUnknown;
            break;
            
    }
    
    return ret;
}

StandardCopyJob::StandardCopyJob(IPlugin* parent, const KUrl& source, const KUrl& dest,
                                 OutputJob::OutputJobVerbosity verbosity)
    : VcsJob(parent, verbosity)
    , m_source(source), m_dest(dest)
    , m_plugin(parent)
    , m_status(JobNotStarted)
{}

void StandardCopyJob::start()
{
    KIO::CopyJob* job=KIO::copy(m_source, m_dest);
    connect(job, SIGNAL(result(KJob*)), SLOT(result(KJob*)));
    job->start();
    m_status=JobRunning;
}

void StandardCopyJob::result(KJob* job)
{
    m_status=job->error() == 0? JobSucceeded : JobFailed; emitResult();
}

VcsJob* GitPlugin::copy(const KUrl& localLocationSrc, const KUrl& localLocationDstn)
{
    //TODO: Probably we should "git add" after
    return new StandardCopyJob(this, localLocationSrc, localLocationDstn, KDevelop::OutputJob::Silent);
}

VcsJob* GitPlugin::move(const KUrl& source, const KUrl& destination)
{
    DVcsJob* job = new DVcsJob(urlDir(source), this, KDevelop::OutputJob::Verbose);
    {
        *job << "git" << "mv" << source.toLocalFile() << destination.toLocalFile();
        return job;
    }
}

void GitPlugin::parseGitRepoLocationOutput(DVcsJob* job)
{
    job->setResults(qVariantFromValue(KUrl(job->output())));
}

VcsJob* GitPlugin::repositoryLocation(const KUrl& localLocation)
{
    DVcsJob* job = new DVcsJob(urlDir(localLocation), this);
    //Probably we should check first if origin is the proper remote we have to use but as a first attempt it works
    *job << "git" << "config" << "remote.origin.url";
    connect(job, SIGNAL(readyForParsing(KDevelop::DVcsJob*)), SLOT(parseGitRepoLocationOutput(KDevelop::DVcsJob*)));
    return job;
}

VcsJob* GitPlugin::pull(const KDevelop::VcsLocation& localOrRepoLocationSrc, const KUrl& localRepositoryLocation)
{
    DVcsJob* job = new DVcsJob(urlDir(localRepositoryLocation), this);
    *job << "git" << "pull" << localOrRepoLocationSrc.localUrl().url();
    return job;
}

VcsJob* GitPlugin::push(const KUrl& localRepositoryLocation, const KDevelop::VcsLocation& localOrRepoLocationDst)
{
    DVcsJob* job = new DVcsJob(urlDir(localRepositoryLocation), this);
    *job << "git" << "push" << localOrRepoLocationDst.localUrl().url();
    return job;
}

VcsJob* GitPlugin::resolve(const KUrl::List& localLocations, IBasicVersionControl::RecursionMode recursion)
{
    return add(localLocations, recursion);
}

VcsJob* GitPlugin::update(const KUrl::List& localLocations, const KDevelop::VcsRevision& rev, IBasicVersionControl::RecursionMode recursion)
{
    if(rev.revisionType()==VcsRevision::Special && rev.revisionValue().value<VcsRevision::RevisionSpecialType>()==VcsRevision::Head) {
        return pull(VcsLocation(), localLocations.first());
    } else {
        DVcsJob* job = new DVcsJob(urlDir(localLocations.first().toLocalFile()), this);
        {
            //Probably we should check first if origin is the proper remote we have to use but as a first attempt it works
            *job << "git" << "checkout" << rev.revisionValue().toString() << "--";
            *job << (recursion == IBasicVersionControl::Recursive ? localLocations : preventRecursion(localLocations));
            return job;
        }
    }
}

class GitVcsLocationWidget : public KDevelop::StandardVcsLocationWidget
{
    public:
        GitVcsLocationWidget(QWidget* parent = 0, Qt::WindowFlags f = 0)
            : StandardVcsLocationWidget(parent, f) {}
        virtual bool isCorrect() const { return true; }
};

KDevelop::VcsLocationWidget* GitPlugin::vcsLocation(QWidget* parent) const
{
    return new GitVcsLocationWidget(parent);
}
