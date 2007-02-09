
#include <config.h>

#include <QtCore/QStringList>

#include "kdevmakebuilder.h"
#include <kdevprojectmodel.h>

#include <kdevproject.h>
#include <kdevcore.h>
#include <kdevplugincontroller.h>
#include <kdevmakeinterface.h>
#include <domutil.h>

#include <kiconloader.h>
#include <kgenericfactory.h>
#include <kprocess.h>
#include <kdialog.h>
#include <kglobal.h>
#include <klocale.h>
#include <kdebug.h>

#define MAKE_COMMAND "make"

K_EXPORT_COMPONENT_FACTORY(kdevmakebuilder, KGenericFactory<KDevMakeBuilder>("kdevmakebuilder"))

KDevMakeBuilder::KDevMakeBuilder(QObject *parent, const QStringList &)
    : KDevelop::ProjectBuilder(parent)
{
    m_project = qobject_cast<KDevelop::Project*>(parent);
    Q_ASSERT(m_project);

    KDevelop::PluginController* kdpc = KDevelop::PluginController::self();
    KDevelop::MakeFrontend* make = kdpc->extension<KDevelop::MakeFrontend>("KDevelop/MakeFrontend");
    if ( make )
    {
        connect(make, SIGNAL(commandFinished(const QString &)),
            this, SLOT(commandFinished(const QString &)));

        connect(make, SIGNAL(commandFailed(const QString &)),
            this, SLOT(commandFailed(const QString &)));
    }
}

KDevMakeBuilder::~KDevMakeBuilder()
{
}

KDevelop::Project *KDevMakeBuilder::project() const
{
    return m_project;
}

bool KDevMakeBuilder::build(KDevelop::ProjectItem *dom)
{
    KDevelop::PluginController* kdpc = KDevelop::PluginController::self();
    if (KDevelop::MakeFrontend *make =
        kdpc->extension<KDevelop::MakeFrontend>("KDevelop/MakeFrontend"))
    {
        if (KDevelop::ProjectFolderItem *folder = dom->folder()) {
            // ### compile the folder
            QString command = buildCommand(dom);
            make->queueCommand(folder->text(), command);
            m_commands.append(qMakePair(command, dom));
            return true;
        } else if (KDevelop::ProjectTargetItem *target = dom->target()) {
            // ### compile the target
            Q_UNUSED(target);
        } else if (KDevelop::ProjectFileItem *file = dom->file()) {
            // ### compile the file
            Q_UNUSED(file);
        }
    }

    return false;
}

bool KDevMakeBuilder::clean(KDevelop::ProjectItem *dom)
{
    Q_UNUSED(dom);
    return false;
}

void KDevMakeBuilder::commandFinished(const QString &command)
{
    if (!m_commands.isEmpty()) {
        QPair<QString, KDevelop::ProjectItem *> item = m_commands.first();
        if (item.first == command) {
            m_commands.pop_front();
            emit built(item.second);
        }
    }
}

void KDevMakeBuilder::commandFailed(const QString &command)
{
    Q_UNUSED(command);

    if (!m_commands.isEmpty()) {
        m_commands.clear();

        emit failed();
    }
}

QString KDevMakeBuilder::buildCommand(KDevelop::ProjectItem *item, const QString &target)
{
    //FIXME Get this from the new project file format
//     QDomDocument &dom = *KDevApi::self()->projectDom();

    QString cmdline;
//     QString cmdline = DomUtil::readEntry(dom, makeTool);
//     int prio = DomUtil::readIntEntry(dom, priority);
//     QString nice;
//     if (prio != 0) {
//         nice = QString("nice -n%1 ").arg(prio);
//     }

    if (cmdline.isEmpty())
        cmdline = MAKE_COMMAND;

//     if (!DomUtil::readBoolEntry(dom, abortOnError))
//         cmdline += " -k";
//     int jobs = DomUtil::readIntEntry(dom, numberOfJobs);
//     if (jobs != 0) {
//         cmdline += " -j";
//         cmdline += QString::number(jobs);
//     }
//     if (DomUtil::readBoolEntry(dom, dontAct))
//         cmdline += " -n";

    cmdline += " ";
    cmdline += target;

//     cmdline.prepend(nice);
//     cmdline.prepend(makeEnvironment());

    Q_ASSERT(item->folder());

    QString dircmd = "cd ";
    QString dir = item->folder()->text();
    dircmd += KProcess::quote(dir);
    dircmd += " && ";

    return dircmd + cmdline;
}

#include "kdevmakebuilder.moc"
