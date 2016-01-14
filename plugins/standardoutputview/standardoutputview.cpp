/* KDevelop Standard OutputView
 *
 * Copyright 2006-2007 Andreas Pakulat <apaku@gmx.de>
 * Copyright 2007 Dukju Ahn <dukjuahn@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include "standardoutputview.h"
#include "outputwidget.h"
#include "toolviewdata.h"
#include "debug.h"

#include <QAbstractItemDelegate>
#include <QAbstractItemModel>
#include <QAction>
#include <QList>

#include <KLocalizedString>

#include <interfaces/icore.h>
#include <interfaces/iuicontroller.h>

#include <sublime/view.h>
#include <sublime/area.h>
#include <sublime/controller.h>
#include <sublime/document.h>

class OutputViewFactory : public KDevelop::IToolViewFactory{
public:
    OutputViewFactory(const ToolViewData* data): m_data(data) {}
    QWidget* create(QWidget *parent = 0) override
    {
        return new OutputWidget( parent, m_data );
    }
    Qt::DockWidgetArea defaultPosition() override
    {
        return Qt::BottomDockWidgetArea;
    }
    void viewCreated( Sublime::View* view ) override
    {
        m_data->views << view;
    }
    QString id() const override
    {
        //NOTE: id must be unique, see e.g. https://bugs.kde.org/show_bug.cgi?id=287093
        return "org.kdevelop.OutputView." + QString::number(m_data->toolViewId);
    }
private:
    const ToolViewData *m_data;
};

StandardOutputView::StandardOutputView(QObject *parent, const QVariantList &)
    : KDevelop::IPlugin(QStringLiteral("kdevstandardoutputview"), parent)
{
    KDEV_USE_EXTENSION_INTERFACE( KDevelop::IOutputView )

    setXMLFile(QStringLiteral("kdevstandardoutputview.rc"));

    connect(KDevelop::ICore::self()->uiController()->controller(), &Sublime::Controller::aboutToRemoveView,
            this, &StandardOutputView::removeSublimeView);

}

void StandardOutputView::removeSublimeView( Sublime::View* v )
{
    foreach( ToolViewData* d, toolviews )
    {
        if( d->views.contains(v) )
        {
            if( d->views.count() == 1 )
            {
                toolviews.remove( d->toolViewId );
                ids.removeAll( d->toolViewId );
                delete d;
            } else
            {
                d->views.removeAll(v);
            }
        }
    }
}

StandardOutputView::~StandardOutputView()
{
}

int StandardOutputView::standardToolView( KDevelop::IOutputView::StandardToolView view )
{
    if( standardViews.contains( view ) )
    {
        return standardViews.value( view );
    }
    int ret = -1;
    switch( view )
    {
        case KDevelop::IOutputView::BuildView:
        {
            ret = registerToolView( i18nc("@title:window", "Build"), KDevelop::IOutputView::HistoryView, QIcon::fromTheme(QStringLiteral("run-build")), KDevelop::IOutputView::AddFilterAction );
            break;
        }
        case KDevelop::IOutputView::RunView:
        {
            ret = registerToolView( i18nc("@title:window", "Run"), KDevelop::IOutputView::MultipleView, QIcon::fromTheme(QStringLiteral("system-run")), KDevelop::IOutputView::AddFilterAction );
            break;
        }
        case KDevelop::IOutputView::DebugView:
        {
            ret = registerToolView( i18nc("@title:window", "Debug"), KDevelop::IOutputView::MultipleView, QIcon::fromTheme(QStringLiteral("debug-step-into")), KDevelop::IOutputView::AddFilterAction );
            break;
        }
        case KDevelop::IOutputView::TestView:
        {
            ret = registerToolView( i18nc("@title:window", "Test"), KDevelop::IOutputView::HistoryView, QIcon::fromTheme(QStringLiteral("system-run")));
            break;
        }
        case KDevelop::IOutputView::VcsView:
        {
            ret = registerToolView( i18nc("@title:window", "Version Control"), KDevelop::IOutputView::HistoryView, QIcon::fromTheme(QStringLiteral("system-run")));
            break;
        }
    }

    Q_ASSERT(ret != -1);
    standardViews[view] = ret;
    return ret;
}

int StandardOutputView::registerToolView( const QString& title,
                                          KDevelop::IOutputView::ViewType type,
                                          const QIcon& icon, Options option,
                                          const QList<QAction*>& actionList )
{
    // try to reuse existing toolview
    foreach( ToolViewData* d, toolviews )
    {
        if ( d->type == type && d->title == title ) {
            return d->toolViewId;
        }
    }

    // register new tool view
    const int newid = ids.isEmpty() ? 0 : (ids.last() + 1);
    qCDebug(PLUGIN_STANDARDOUTPUTVIEW) << "Registering view" << title << "with type:" << type << "id:" << newid;
    ToolViewData* tvdata = new ToolViewData( this );
    tvdata->toolViewId = newid;
    tvdata->type = type;
    tvdata->title = title;
    tvdata->icon = icon;
    tvdata->plugin = this;
    tvdata->option = option;
    tvdata->actionList = actionList;
    core()->uiController()->addToolView( title, new OutputViewFactory( tvdata ) );
    ids << newid;
    toolviews[newid] = tvdata;
    return newid;
}

int StandardOutputView::registerOutputInToolView( int toolViewId,
                                                  const QString& title,
                                                  KDevelop::IOutputView::Behaviours behaviour )
{
    if( !toolviews.contains( toolViewId ) )
        return -1;
    int newid;
    if( ids.isEmpty() )
    {
        newid = 0;
    } else
    {
        newid = ids.last()+1;
    }
    ids << newid;
    toolviews.value( toolViewId )->addOutput( newid, title, behaviour );
    return newid;
}

void StandardOutputView::raiseOutput(int outputId)
{
    foreach( int _id, toolviews.keys() )
    {
        if( toolviews.value( _id )->outputdata.contains( outputId ) )
        {
            foreach( Sublime::View* v, toolviews.value( _id )->views ) {
                if( v->hasWidget() )
                {
                    OutputWidget* w = qobject_cast<OutputWidget*>( v->widget() );
                    w->raiseOutput( outputId );
                    v->requestRaise();
                }
            }
        }
    }
}

void StandardOutputView::setModel( int outputId, QAbstractItemModel* model )
{
    int tvid = -1;
    foreach( int _id, toolviews.keys() )
    {
        if( toolviews.value( _id )->outputdata.contains( outputId ) )
        {
            tvid = _id;
            break;
        }
    }
    if( tvid == -1 )
        qCDebug(PLUGIN_STANDARDOUTPUTVIEW) << "Trying to set model on unknown view-id:" << outputId;
    else
    {
        toolviews.value( tvid )->outputdata.value( outputId )->setModel( model );
    }
}

void StandardOutputView::setDelegate( int outputId, QAbstractItemDelegate* delegate )
{
    int tvid = -1;
    foreach( int _id, toolviews.keys() )
    {
        if( toolviews.value( _id )->outputdata.contains( outputId ) )
        {
            tvid = _id;
            break;
        }
    }
    if( tvid == -1 )
        qCDebug(PLUGIN_STANDARDOUTPUTVIEW) << "Trying to set model on unknown view-id:" << outputId;
    else
    {
        toolviews.value( tvid )->outputdata.value( outputId )->setDelegate( delegate );
    }
}

void StandardOutputView::removeToolView( int toolviewId )
{
    if( toolviews.contains(toolviewId) )
    {
        ToolViewData* td = toolviews.value(toolviewId);
        foreach( Sublime::View* view, td->views )
        {
            if( view->hasWidget() )
            {
                OutputWidget* outputWidget = qobject_cast<OutputWidget*>( view->widget() );
                foreach( int outid, td->outputdata.keys() )
                {
                    outputWidget->removeOutput( outid );
                }
            }
            foreach( Sublime::Area* area, KDevelop::ICore::self()->uiController()->controller()->allAreas() )
            {
                area->removeToolView( view );
            }
        }
        delete td;
        toolviews.remove(toolviewId);
        emit toolViewRemoved(toolviewId);
    }
}

OutputWidget* StandardOutputView::outputWidgetForId( int outputId ) const
{
    foreach( ToolViewData* td, toolviews )
    {
        if( td->outputdata.contains( outputId ) )
        {
            foreach( Sublime::View* view, td->views )
            {
                if( view->hasWidget() )
                    return qobject_cast<OutputWidget*>( view->widget() );
            }
        }
    }
    return 0;
}

void StandardOutputView::scrollOutputTo( int outputId, const QModelIndex& idx )
{
    OutputWidget* widget = outputWidgetForId( outputId );
    if( widget )
        widget->scrollToIndex( idx );
}

void StandardOutputView::removeOutput( int outputId )
{
    foreach( ToolViewData* td, toolviews )
    {
        if( td->outputdata.contains( outputId ) )
        {
            foreach( Sublime::View* view, td->views )
            {
                if( view->hasWidget() )
                    qobject_cast<OutputWidget*>( view->widget() )->removeOutput( outputId );
            }
            td->outputdata.remove( outputId );
        }
    }
}

void StandardOutputView::setTitle(int outputId, const QString& title)
{
    outputWidgetForId(outputId)->setTitle(outputId, title);
}
