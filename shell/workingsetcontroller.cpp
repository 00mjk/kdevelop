/*
    Copyright David Nolden  <david.nolden.kdevelop@art-master.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "workingsetcontroller.h"

#include <QTimer>
#include <QVBoxLayout>

#include "mainwindow.h"
#include "partdocument.h"
#include "uicontroller.h"

#include <interfaces/iuicontroller.h>
#include <interfaces/isession.h>

#include <sublime/view.h>
#include <sublime/area.h>

#include <util/activetooltip.h>

#include "workingsets/workingset.h"
#include "workingsets/workingsettooltipwidget.h"
#include "workingsets/workingsetwidget.h"
#include "workingsets/closedworkingsetswidget.h"

using namespace KDevelop;

const int toolTipTimeout = 2000;

//Random set of icons that are well distinguishable from each other. If the user doesn't have them, they won't be used.
QStringList setIcons = QStringList() << "chronometer" << "games-config-tiles" << "im-user" << "irc-voice" << "irc-operator" << "office-chart-pie" << "office-chart-ring" << "speaker" << "view-pim-notes" << "esd" << "akonadi" << "kleopatra" << "nepomuk" << "package_edutainment_art" << "package_games_amusement" << "package_games_sports" << "package_network" << "package_office_database" << "package_system_applet" << "package_system_emulator" << "preferences-desktop-notification-bell" << "wine" << "utilities-desktop-extra" << "step" << "preferences-web-browser-cookies" << "preferences-plugin" << "preferences-kcalc-constants" << "preferences-desktop-icons" << "tagua" << "inkscape" << "java" << "kblogger" << "preferences-desktop-personal" << "emblem-favorite" << "face-smile-big" << "face-embarrassed" << "user-identity" << "mail-tagged" << "media-playlist-suffle" << "weather-clouds";

WorkingSetController::WorkingSetController(Core* core)
    : m_core(core)
{
    m_hideToolTipTimer = new QTimer(this);
    m_hideToolTipTimer->setInterval(toolTipTimeout);
    m_hideToolTipTimer->setSingleShot(true);
}

void WorkingSetController::initialize()
{
    //Load all working-sets
    KConfigGroup setConfig(Core::self()->activeSession()->config(), "Working File Sets");
    foreach(const QString& set, setConfig.groupList())
    {
        if(setConfig.group(set).hasKey("iconName"))
            getWorkingSet(set, setConfig.group(set).readEntry<QString>("iconName", QString()));
        else
            kDebug() << "have garbage working set with id " << set;
    }

    if(!(Core::self()->setupFlags() & Core::NoUi)) setupActions();
}

void WorkingSetController::cleanup()
{
    foreach(Sublime::MainWindow* window, Core::self()->uiControllerInternal()->mainWindows()) {
        foreach (Sublime::Area *area, window->areas()) {
            if (!area->workingSet().isEmpty()) {
                Q_ASSERT(m_workingSets.contains(area->workingSet()));
                m_workingSets[area->workingSet()]->saveFromArea(area, area->rootIndex());
            }
        }
    }

    foreach(WorkingSet* set, m_workingSets) {
        kDebug() << "set" << set->id() << "persistent" << set->isPersistent() << "has areas:" << set->hasConnectedAreas() << "files" << set->fileList();
        if(!set->isPersistent() && !set->hasConnectedAreas()) {
            kDebug() << "deleting";
            set->deleteSet(true, true);
        }
        delete set;
    }

    m_workingSets.clear();
}

bool WorkingSetController::usingIcon(const QString& icon)
{
    foreach(WorkingSet* set, m_workingSets)
        if(set->iconName() == icon)
            return true;
    return false;
}

bool WorkingSetController::iconValid(const QString& icon)
{
    return !KIconLoader::global()->iconPath(icon, KIconLoader::Small, true).isNull();
}

WorkingSet* WorkingSetController::newWorkingSet(const QString& prefix)
{
    QString newId = QString("%1_%2").arg(prefix).arg(qrand() % 10000000);
    return getWorkingSet(newId);
}

WorkingSet* WorkingSetController::getWorkingSet(const QString& id, const QString& _icon)
{
    Q_ASSERT(!id.isEmpty());

    if(!m_workingSets.contains(id)) {
        QString icon = _icon;
        if(icon.isEmpty())
        {
            for(int a = 0; a < 100; ++a) {
                int pick = (qHash(id) + a) % setIcons.size(); ///@todo Pick icons semantically, by content, and store them in the config
                if(!usingIcon(setIcons[pick])) {
                    if(iconValid(setIcons[pick])) {
                        icon = setIcons[pick];
                    break;
                    }
                }
            }
        }
        if(icon.isEmpty()) {
            kDebug() << "found no icon for working-set" << id;
            icon = "invalid";
        }
        WorkingSet* set = new WorkingSet(id, icon);
        connect(set, SIGNAL(aboutToRemove(WorkingSet*)),
                this, SIGNAL(aboutToRemoveWorkingSet(WorkingSet*)));
        m_workingSets[id] = set;
        emit workingSetAdded(set);
    }

    return m_workingSets[id];
}

QWidget* WorkingSetController::createSetManagerWidget(MainWindow* parent, Sublime::Area* fixedArea) {
    if (fixedArea) {
        return new WorkingSetWidget(parent, fixedArea);
    } else {
        return new ClosedWorkingSetsWidget(parent);
    }
}

void WorkingSetController::setupActions()
{
/*
    KActionCollection * ac =
        Core::self()->uiControllerInternal()->defaultMainWindow()->actionCollection();

    KAction *action;

    action = ac->addAction ( "view_next_window" );
    action->setText( i18n( "Next Document" ) );
    action->setIcon( KIcon("go-next") );
    action->setShortcut( Qt::ALT + Qt::SHIFT + Qt::Key_Right );
    action->setWhatsThis( i18n( "Switch the focus to the next open document." ) );
    action->setStatusTip( i18n( "Switch the focus to the next open document." ) );
    connect( action, SIGNAL(triggered()), this, SLOT(nextDocument()) );

    action = ac->addAction ( "view_previous_window" );
    action->setText( i18n( "Previous Document" ) );
    action->setIcon( KIcon("go-previous") );
    action->setShortcut( Qt::ALT + Qt::SHIFT + Qt::Key_Left );
    action->setWhatsThis( i18n( "Switch the focus to the previous open document." ) );
    action->setStatusTip( i18n( "Switch the focus to the previous open document." ) );
    connect( action, SIGNAL(triggered()), this, SLOT(previousDocument()) );
*/
}

ActiveToolTip* WorkingSetController::tooltip() const
{
    return m_tooltip;
}

void WorkingSetController::showToolTip(WorkingSet* set, const QPoint& pos)
{
    delete m_tooltip;

    KDevelop::MainWindow* window = static_cast<KDevelop::MainWindow*>(Core::self()->uiControllerInternal()->activeMainWindow());

    m_tooltip = new KDevelop::ActiveToolTip(window, pos);
    QVBoxLayout* layout = new QVBoxLayout(m_tooltip);
    layout->setMargin(0);
    WorkingSetToolTipWidget* widget = new WorkingSetToolTipWidget(m_tooltip, set, window);
    layout->addWidget(widget);
    m_tooltip->resize( m_tooltip->sizeHint() );

    connect(widget, SIGNAL(shouldClose()), m_tooltip, SLOT(close()));

    ActiveToolTip::showToolTip(m_tooltip);
}

void WorkingSetController::showGlobalToolTip()
{
    KDevelop::MainWindow* window = static_cast<KDevelop::MainWindow*>(Core::self()->uiControllerInternal()->activeMainWindow());

    showToolTip(getWorkingSet(window->area()->workingSet()),
                              window->mapToGlobal(window->geometry().topRight()));

    connect(m_hideToolTipTimer, SIGNAL(timeout()),  m_tooltip, SLOT(deleteLater()));
    m_hideToolTipTimer->start();
    connect(m_tooltip, SIGNAL(mouseIn()), m_hideToolTipTimer, SLOT(stop()));
    connect(m_tooltip, SIGNAL(mouseOut()), m_hideToolTipTimer, SLOT(start()));
}

void WorkingSetController::nextDocument()
{
    if(!m_tooltip)
        showGlobalToolTip();

    m_hideToolTipTimer->stop();
    m_hideToolTipTimer->start(toolTipTimeout);

    if(m_tooltip)
    {
        WorkingSetToolTipWidget* widget = m_tooltip->findChild<WorkingSetToolTipWidget*>();
        Q_ASSERT(widget);
        widget->nextDocument();
    }
}

void WorkingSetController::previousDocument()
{
    if(!m_tooltip)
        showGlobalToolTip();

    m_hideToolTipTimer->stop();
    m_hideToolTipTimer->start(toolTipTimeout);

    if(m_tooltip)
    {
        WorkingSetToolTipWidget* widget = m_tooltip->findChild<WorkingSetToolTipWidget*>();
        Q_ASSERT(widget);
        widget->previousDocument();
    }
}

void WorkingSetController::initializeController( UiController* controller )
{
  connect( controller, SIGNAL( areaCreated( Sublime::Area* ) ), this, SLOT( areaCreated( Sublime::Area* ) ) );
}

QList< WorkingSet* > WorkingSetController::allWorkingSets() const
{
  return m_workingSets.values();
}

void WorkingSetController::areaCreated( Sublime::Area* area )
{
    if (!area->workingSet().isEmpty()) {
        WorkingSet* set = getWorkingSet( area->workingSet() );
        set->connectArea( area );
    }

    connect(area, SIGNAL(changingWorkingSet(Sublime::Area*,QString,QString)),
            this, SLOT(changingWorkingSet(Sublime::Area*,QString,QString)));
    connect(area, SIGNAL(changedWorkingSet(Sublime::Area*,QString,QString)),
            this, SLOT(changedWorkingSet(Sublime::Area*,QString,QString)));
    connect(area, SIGNAL(viewAdded(Sublime::AreaIndex*,Sublime::View*)),
            this, SLOT(viewAdded(Sublime::AreaIndex*,Sublime::View*)));
}

void WorkingSetController::changingWorkingSet(Sublime::Area* area, const QString& from, const QString& to)
{
    kDebug() << "changing working-set from" << from << "to" << to << "area" << area;
    if (from == to)
        return;

    if (!from.isEmpty()) {
        WorkingSet* oldSet = getWorkingSet(from);
        oldSet->disconnectArea(area);
        if (!oldSet->id().isEmpty()) {
            oldSet->saveFromArea(area, area->rootIndex());
        }
    }

    if (!to.isEmpty()) {
        WorkingSet* newSet = getWorkingSet(to);
        newSet->connectArea(area);
    }

    kDebug() << "update ready";
}

void WorkingSetController::changedWorkingSet(Sublime::Area* area, const QString& from, const QString& to)
{
    kDebug() << "changed working-set from" << from << "to" << to << "area" << area;
    if (from == to)
        return;

    if (!to.isEmpty()) {
        WorkingSet* newSet = getWorkingSet(to);
        newSet->loadToArea(area, area->rootIndex(), !from.isEmpty());
    } else {
        area->clearViews();
    }

    emit workingSetSwitched();
    kDebug() << "update ready";
}

void WorkingSetController::viewAdded( Sublime::AreaIndex* , Sublime::View* )
{
    Sublime::Area* area = qobject_cast< Sublime::Area* >(sender());
    Q_ASSERT(area);

    if (area->workingSet().isEmpty()) {
        //Spawn a new working-set
        WorkingSet* set = Core::self()->workingSetControllerInternal()->newWorkingSet(area->objectName());
        set->connectArea(area);
        set->saveFromArea(area, area->rootIndex());
        area->setWorkingSet(set->id());
    }
}


#include "workingsetcontroller.moc"
