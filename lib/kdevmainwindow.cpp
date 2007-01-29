/* This file is part of the KDevelop project
Copyright (C) 2002 F@lk Brettschneider <falkbr@kdevelop.org>
Copyright (C) 2003 John Firebaugh <jfirebaugh@kde.org>
Copyright (C) 2006 Adam Treat <treat@kde.org>

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
#include "kdevmainwindow.h"

#include <QHash>
#include <QDockWidget>
#include <QStackedWidget>
#include <QApplication>

#include <kmenu.h>
#include <klocale.h>

#include <kparts/part.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/editor.h>

#include <kstandardaction.h>
#include <kselectaction.h>
#include <ktoggleaction.h>
#include <kxmlguiclient.h>
#include <kxmlguifactory.h>
#include <kactioncollection.h>
#include <ktoolbarpopupaction.h>

#include <knotifyconfigwidget.h>

#include "kdevcore.h"
#include "kdevconfig.h"
#include "kdevplugin.h"
#include "kdevprofile.h"
#include "kdevdocument.h"
#include "kdevstatusbar.h"
#include "shellextension.h"
#include "kdevprofileengine.h"
#include "kdevplugincontroller.h"
#include "kdevprojectcontroller.h"
#include "kdevdocumentcontroller.h"

namespace Koncrete
{


class MainWindowPrivate
{
public:
    MainWindowPrivate()
            : mode( MainWindow::DockedMode ),
            center( 0 )
    {}

    MainWindow::UIMode mode;

    QStackedWidget *center;
    QPointer<QWidget> centralPlugin;

    QList<Plugin*> activeProcesses;
};

MainWindow::MainWindow( QWidget *parent, Qt::WFlags flags )
        : KMainWindow( parent, flags )
{
    setObjectName( QLatin1String( "MainWindow" ) );
    d = new MainWindowPrivate();
    d->center = new QStackedWidget( this );
    setCentralWidget( d->center );

    setCorner( Qt::TopLeftCorner, Qt::LeftDockWidgetArea );
    setCorner( Qt::TopRightCorner, Qt::RightDockWidgetArea );
    setCorner( Qt::BottomLeftCorner, Qt::LeftDockWidgetArea );
    setCorner( Qt::BottomRightCorner, Qt::RightDockWidgetArea );

    setStandardToolBarMenuEnabled( true );
    setupActions();
    setStatusBar( new Koncrete::StatusBar( this ) );

    connect( PluginController::self(), SIGNAL(pluginLoaded(Plugin*)),
             this, SLOT(addPlugin(Plugin*)));

/*    createGUI( ShellExtension::getInstance() ->xmlFile() );*/
}

MainWindow::~ MainWindow()
{}

MainWindow::UIMode MainWindow::mode() const
{
    return d->mode;
}

void MainWindow::setUIMode( UIMode mode )
{
    switch ( mode )
    {
    case TopLevelMode:
        switchToTopLevelMode();
        break;
    case DockedMode:
        switchToDockedMode();
        break;

    default: Q_ASSERT( 0 );
    }
}

void MainWindow::setupActions()
{
    KStandardAction::quit( this, SLOT( close() ), actionCollection() );

    QAction *action;

    QString app = qApp->applicationName();
    QString text = i18n( "Configure %1", app );
    action = KStandardAction::preferences( this, SLOT( settingsDialog() ),
                                      actionCollection());
    actionCollection()->addAction( "settings_configure", action );
    action->setToolTip( text );
    action->setWhatsThis( QString( "<b>%1</b><p>%2" ).arg( text ).arg(
                              i18n( "Lets you customize %1.", app ) ) );

    action = actionCollection()->addAction( "settings_configure_editors" );
    action->setText( i18n( "Configure &Editor..." ) );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( configureEditors() ) );
    action->setToolTip( i18n( "Configure editor settings" ) );
    action->setWhatsThis( i18n( "<b>Configure editor</b><p>Opens editor configuration dialog." ) );
    action->setEnabled( false );

    action = KStandardAction::showStatusbar( this, SLOT( toggleStatusbar() ),
                                        actionCollection());
    actionCollection()->addAction( "settings_show_statusbar", action );
    action->setText( i18n( "Show &Statusbar" ) );
    action->setToolTip( i18n( "Show statusbar" ) );
    action->setWhatsThis( i18n( "<b>Show statusbar</b><p>Hides or shows the statusbar." ) );

    KToolBarPopupAction *popupAction;
    popupAction = new KToolBarPopupAction( KIcon( "stop" ),
                                           i18n( "&Stop" ),
                                           actionCollection()
                                           );
    actionCollection()->addAction( "stop_processes",popupAction );

    popupAction->setShortcut( Qt::Key_Escape );
    popupAction->setToolTip( i18n( "Stop" ) );
    popupAction->setWhatsThis( i18n( "<b>Stop</b><p>Stops all running processes." ) );
    popupAction->setEnabled( false );
    connect( popupAction, SIGNAL( triggered() ),
             this, SLOT( stopButtonPressed() ) );
    connect( popupAction->menu(), SIGNAL( aboutToShow() ),
             this, SLOT( stopMenuAboutToShow() ) );
    connect( popupAction->menu(), SIGNAL( activated( int ) ),
             this, SLOT( stopPopupActivated( int ) ) );

    //FIXME fix connection after gutting of Core
    /*connect( Core::getInstance(), SIGNAL( activeProcessChanged( Plugin*, bool ) ),
    this, SLOT( activeProcessChanged( Plugin*, bool ) ) );*/

    action = KStandardAction::showMenubar(
                 this, SLOT( showMenuBar() ),
                 actionCollection());
    actionCollection()->addAction( "settings_show_menubar", action );
    action->setToolTip( beautifyToolTip( action->text() ) );
    action->setWhatsThis( QString( "<b>%1</b><p>%2" ).arg( beautifyToolTip( action->text() ) ).arg( i18n( "Lets you toggle the menubar on/off." ) ) );

    action = KStandardAction::keyBindings(
                 this, SLOT( keyBindings() ),
                 actionCollection());
    actionCollection()->addAction( "settings_configure_shortcuts", action );
    action->setToolTip( beautifyToolTip( action->text() ) );
    action->setWhatsThis( QString( "<b>%1</b><p>%2" ).arg( beautifyToolTip( action->text() ) ).arg( i18n( "Lets you configure shortcut keys." ) ) );

    action = KStandardAction::configureToolbars(
                 this, SLOT( configureToolbars() ),
                 actionCollection() );
    actionCollection()->addAction( "settings_configure_toolbars", action );
    action->setToolTip( beautifyToolTip( action->text() ) );
    action->setWhatsThis( QString( "<b>%1</b><p>%2" ).arg( beautifyToolTip( action->text() ) ).arg( i18n( "Lets you configure toolbars." ) ) );

    action = KStandardAction::configureNotifications(
                 this, SLOT( configureNotifications() ),
                 actionCollection());
    actionCollection()->addAction( "settings_configure_notifications" );
    action->setToolTip( beautifyToolTip( action->text() ) );
    action->setWhatsThis( QString( "<b>%1</b><p>%2" ).arg( beautifyToolTip( action->text() ) ).arg( i18n( "Lets you configure system notifications." ) ) );

    action = actionCollection()->addAction( "view_next_window" );
    action->setText( i18n( "&Next Window" ) );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( gotoNextWindow() ) );
    action->setShortcut( Qt::ALT + Qt::Key_Right );
    action->setToolTip( i18n( "Next window" ) );
    action->setWhatsThis( i18n( "<b>Next window</b><p>Switches to the next window." ) );

    action = actionCollection()->addAction( "view_previous_window" );
    action->setText( i18n( "&Previous Window" ) );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( gotoPreviousWindow() ) );
    action->setShortcut( Qt::ALT + Qt::Key_Left );
    action->setToolTip( i18n( "Previous window" ) );
    action->setWhatsThis( i18n( "<b>Previous window</b><p>Switches to the previous window." ) );

    action = actionCollection()->addAction( "view_last_window" );
    action->setText( i18n( "&Last Accessed Window" ) );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( gotoLastWindow() ) );
    action->setShortcut( Qt::ALT + Qt::Key_Up );
    action->setToolTip( i18n( "Last accessed window" ) );
    action->setWhatsThis( i18n( "<b>Last accessed window</b><p>Switches to the last viewed window (Hold the Alt key pressed and walk on by repeating the Up key)." ) );

    action = actionCollection()->addAction( "view_first_window" );
    action->setText( i18n( "&First Accessed Window" ) );
    connect( action, SIGNAL( triggered( bool ) ), SLOT( gotoFirstWindow() ) );
    action->setShortcut( Qt::ALT + Qt::Key_Down );
    action->setToolTip( i18n( "First accessed window" ) );
    action->setWhatsThis( i18n( "<b>First accessed window</b><p>Switches to the first accessed window (Hold the Alt key pressed and walk on by repeating the Down key)." ) );
}

void MainWindow::loadSettings( bool projectIsLoaded )
{
    Q_UNUSED( projectIsLoaded );
    KConfig * config = Config::standard();

    config->setGroup( "UI" );
    bool docked = config->readEntry( "Docked Window", true );
    bool toplevel = config->readEntry( "Multiple Top-Level Windows", false );

    //FIXME this needs to be an enum value from kconfigxt
    if ( docked )
        setUIMode( DockedMode );
    if ( toplevel )
        setUIMode( TopLevelMode );

    applyMainWindowSettings( config, QLatin1String( "MainWindow" ) );
}

void MainWindow::saveSettings( bool projectIsLoaded )
{
    if ( projectIsLoaded )
        return;

    KConfig * config = Config::standard();

    saveMainWindowSettings( config, QLatin1String( "MainWindow" ) );
}

void MainWindow::initialize()
{
    createGUI( ShellExtension::getInstance() ->xmlFile() );
    connect( Core::documentController(), SIGNAL( documentActivated( Document* ) ),
             this, SLOT( documentActivated( Document* ) ) );
    connect( Core::projectController(), SIGNAL( projectOpened() ),
             this, SLOT( projectOpened() ) );
    connect( Core::projectController(), SIGNAL( projectClosed() ),
             this, SLOT( projectClosed() ) );
}

void MainWindow::cleanup()
{
}

void MainWindow::fillContextMenu( KMenu *menu, const Context *context )
{
    //Perhaps we get rid of this framework and instead have every Context contains
    //a kactioncollection.  Plugins could add their actions directly to the context
    //object retrieved from Core... ??
    emit contextMenu( menu, context );

    //Put this in every context menu so that plugins will be encouraged to allow shortcuts
    QAction * action = actionCollection() ->action( "settings_configure_shortcuts" );
    menu->addAction( action );
}

void MainWindow::addDocument( Document *document )
{
    Q_ASSERT( document );
    Q_ASSERT( document->part() );
    Q_ASSERT( document->part() ->widget() );

    QWidget *widget = document->part() ->widget();
    widget->setWindowTitle( document->url().fileName() );

    switch ( d->mode )
    {
    case TopLevelMode:
        {
            widget->setParent( magicalParent(), magicalWindowFlags( widget ) );
            widget->setMinimumSize( 640, 480 );
            widget->raise();
            widget->show();
            widget->activateWindow();
        }
        break;
    case DockedMode:
        d->center->addWidget( widget );
        widget->show();
        widget->setFocus();
        break;
    case NeutralMode:
        break;
    default:
        break;
    }
}

bool MainWindow::containsDocument( Document *document ) const
{
    Q_ASSERT( document );
    Q_ASSERT( document->part() );
    Q_ASSERT( document->part() ->widget() );

    switch ( d->mode )
    {
    case TopLevelMode:
        return true; //FIXME
    case DockedMode:
        return ( d->center->indexOf( document->part() ->widget() ) != -1 );
    case NeutralMode:
        return true; //FIXME
    default:
        return false;
    }
}

void MainWindow::setCurrentDocument( Document *document )
{
    Q_ASSERT( document );
    Q_ASSERT( document->part() );
    Q_ASSERT( document->part() ->widget() );

    QWidget *widget = document->part() ->widget();
    widget->setWindowTitle( document->url().fileName() );

    switch ( d->mode )
    {
    case TopLevelMode:
        widget->setMinimumSize( 640, 480 );
        widget->raise();
        widget->show();
        widget->activateWindow();
        break;
    case DockedMode:
        d->center->setCurrentWidget( widget );
        widget->show();
        widget->setFocus();
        break;
    case NeutralMode:
        break;
    default:
        break;
    }
}

void MainWindow::removeDocument( Document *document )
{
    Q_ASSERT( document );
    Q_ASSERT( document->part() );
    Q_ASSERT( document->part() ->widget() );

    switch ( d->mode )
    {
    case TopLevelMode:             //FIXME
        break;
    case DockedMode:
        d->center->removeWidget( document->part() ->widget() );
        break;
    case NeutralMode:
        break;
    default:
        break;
    }
}

void MainWindow::addPlugin( Plugin *plugin )
{
    Q_ASSERT( plugin );

    guiFactory()->addClient( plugin );
    QWidget *view = plugin->pluginView();

    //Plugin has no view. Ignore.
    if ( !plugin->pluginView()
            || plugin->dockWidgetAreaHint() == Qt::NoDockWidgetArea )
        return ;

    // This is required as documented in Plugin
    Q_ASSERT( !view->objectName().isEmpty() );
    // This is required as documented in Plugin
    Q_ASSERT( !view->windowTitle().isEmpty() );

    switch ( d->mode )
    {
    case TopLevelMode:
        {
            QDockWidget * dockWidget = magicalDockWidget( view );
            if ( dockWidget == 0 )
            {
                dockWidget = new QDockWidget( this );
                dockWidget->setObjectName( view->objectName() + QLatin1String( "_dock" ) );
                dockWidget->setWindowTitle( view->windowTitle() );
                addDockWidget( plugin->dockWidgetAreaHint(), dockWidget );
                dockWidget->hide();
            }

            if ( plugin->isCentralPlugin() )
            {
                d->centralPlugin = view;
                QRect g = d->centralPlugin->geometry();
                d->center->addWidget( d->centralPlugin );
                d->centralPlugin->show();
                setGeometry( g );
                break;
            }

            view->setParent( magicalParent(), magicalWindowFlags( view ) );
            view->setMinimumSize( 200, 200 ); //FIXME do better geometry and move the windows
            view->show();
            break;
        }
    case DockedMode:
        {
            QDockWidget *dockWidget = magicalDockWidget( view );
            if ( dockWidget == 0 )
            {
                dockWidget = new QDockWidget( this );
                dockWidget->setObjectName( view->objectName() + QLatin1String( "_dock" ) );
                dockWidget->setWindowTitle( view->windowTitle() );
                addDockWidget( plugin->dockWidgetAreaHint(), dockWidget );
            }
            dockWidget->setWidget( view );

            if ( plugin->isCentralPlugin() )
            {
                d->centralPlugin = view;
            }
            break;
        }
    case NeutralMode:
        break;
    default:
        break;
    }
}

void MainWindow::removePlugin( Plugin *plugin )
{
    Q_ASSERT( plugin );

    guiFactory()->removeClient( plugin );
    QWidget *view = plugin->pluginView();

    //Plugin has no view. Ignore.
    if ( !plugin->pluginView()
            || plugin->dockWidgetAreaHint() == Qt::NoDockWidgetArea )
        return ;

    switch ( d->mode )
    {
    case TopLevelMode:
        break;
    case DockedMode:
        {
            QDockWidget *dockWidget = magicalDockWidget( view );
            if ( dockWidget != 0 )
            {
                removeDockWidget( dockWidget );
                delete dockWidget;
            }
            break;
        }
    case NeutralMode:
        break;
    default:
        break;
    }
}

void MainWindow::setVisible( bool visible )
{
    KMainWindow::setVisible( visible );

    emit finishedLoading();
}

void MainWindow::gotoNextWindow()
{
    if ( ( d->center->currentIndex() + 1 ) < d->center->count() )
        d->center->setCurrentIndex( d->center->currentIndex() + 1 );
    else
        d->center->setCurrentIndex( 0 );
}

void MainWindow::gotoPreviousWindow()
{
    if ( ( d->center->currentIndex() - 1 ) >= 0 )
        d->center->setCurrentIndex( d->center->currentIndex() - 1 );
    else
        d->center->setCurrentIndex( d->center->count() - 1 );
}

void MainWindow::gotoFirstWindow()
{
    d->center->setCurrentIndex( 0 );
}

void MainWindow::gotoLastWindow()
{
    d->center->setCurrentIndex( d->center->count() - 1 );
}

void MainWindow::projectOpened()
{
    QString app = i18n( "Project" );
    QString text = i18n( "Configure %1", app );
    QAction *action = actionCollection() ->action( "settings_configure" );
    action->setToolTip( text );
    action->setWhatsThis( QString( "<b>%1</b><p>%2" ).arg( text ).arg(
                              i18n( "Lets you customize %1.", app ) ) );
}

void MainWindow::projectClosed()
{
    QString app = qApp->applicationName();
    QString text = i18n( "Configure %1", app );
    QAction *action = actionCollection() ->action( "settings_configure" );
    action->setToolTip( text );
    action->setWhatsThis( QString( "<b>%1</b><p>%2" ).arg( text ).arg(
                              i18n( "Lets you customize %1.", app ) ) );
}

void MainWindow::configureToolbars()
{}

void MainWindow::newToolbarConfig()
{
    applyMainWindowSettings( KGlobal::config().data(),
                             QLatin1String( "MainWindow" ) );
}

bool MainWindow::queryClose()
{
    PluginController::self()->shutdown();
    //All Core API objects must release all resources which
    //depend upon one another.
    Core::cleanup();

    return true;
}

void MainWindow::setupWindowMenu()
{
    //FIXME This should setup a window menu or perhaps dialog instead.
    // Either way, we need one with a scroll bar.  I'm tired of menus that take
    // up the entire screen.
}

void MainWindow::fillWindowMenu()
{
    //FIXME This should fill a window menu or perhaps dialog instead.
    // Either way, we need one with a scroll bar.  I'm tired of menus that take
    // up the entire screen.
}

QString MainWindow::beautifyToolTip( const QString& text ) const
{
    QString temp = text;
    temp.replace( QRegExp( "&" ), "" );
    temp.replace( QRegExp( "\\.\\.\\." ), "" );
    return temp;
}

void MainWindow::reportBug()
{}

void MainWindow::toggleStatusbar()
{
    KToggleAction * action =
        qobject_cast< KToggleAction*>( actionCollection() ->action( "settings_show_statusbar" ) );
    statusBar() ->setHidden( !action->isChecked() );
}

void MainWindow::stopButtonPressed()
{}

void MainWindow::activeProcessChanged( Plugin* plugin, bool active )
{
    Q_UNUSED( plugin );
    Q_UNUSED( active );
}

void MainWindow::stopPopupActivated( int id )
{
    Q_UNUSED( id );
}

void MainWindow::stopMenuAboutToShow()
{}

void MainWindow::showMenuBar()
{}

void MainWindow::configureNotifications()
{
    KNotifyConfigWidget::configure( this, "Notification Configuration Dialog" );
}


void MainWindow::settingsDialog()
{
    Config::settingsDialog();
}

void MainWindow::configureEditors()
{
    //FIXME Change this so that it is embedded in our config dialog.
    //Perhaps this will require a change to the KTextEditor interface too...
    KTextEditor::Document * doc =
        Core::documentController() ->activeDocument() ->textDocument();
    KTextEditor::Editor *editor = doc ? doc->editor() : 0;
    if ( !editor )
    {
        return ;
    }

    if ( !editor->configDialogSupported() )
    {
        kDebug( 9000 ) << "KTextEditor::configDialogSupported() == false" << endl;
    }

    // show the modal config dialog for this part if it has a ConfigInterface
    editor->configDialog( this );
}

void MainWindow::keyBindings()
{}

void MainWindow::documentActivated( Document *document )
{
    QAction * action = actionCollection() ->action( "settings_configure_editors" );
    action->setEnabled( document->textDocument() );
}

QWidget *MainWindow::magicalParent() const
{
    switch ( d->mode )
    {
    case TopLevelMode:
        return 0;
    case DockedMode:
        return const_cast<MainWindow*>( this );
    case NeutralMode:
        return 0;
    default:
        return 0;
    }
}

QWidget *MainWindow::magicalWidget( QDockWidget *dockWidget ) const
{
    QString name = dockWidget->objectName();
    name.chop( 5 ); //remove the "_dock"

    switch ( d->mode )
    {
    case TopLevelMode:
        {
            QWidget * widget = qFindChild<QWidget*>( this, name );

            if ( widget )
                return widget;
            else
                return 0;
        }
    case DockedMode:
        {
            foreach ( QWidget * widget, QApplication::topLevelWidgets() )
            {
                if ( widget->objectName() == name )
                    return widget;
            }

            //Must be ...
            return d->centralPlugin;
        }
    case NeutralMode:
        return 0;
    default:
        return 0;
    }
}

QDockWidget *MainWindow::magicalDockWidget( QWidget *widget ) const
{
    QDockWidget * dockWidget = qFindChild<QDockWidget*>( this,
                               widget->objectName() + QLatin1String( "_dock" ) );

    if ( dockWidget )
        return dockWidget;
    else
        return 0;
}

Qt::WindowFlags MainWindow::magicalWindowFlags( const QWidget *widgetForFlags ) const
{
    switch ( d->mode )
    {
    case TopLevelMode:
        {
#ifdef Q_WS_MAC
            if ( qobject_cast<const QDockWidget *>( widgetForFlags ) )
                return Qt::Tool;
#else
            Q_UNUSED( widgetForFlags );
#endif
            return Qt::Window;
        }
    case DockedMode:
        return Qt::Window | Qt::WindowShadeButtonHint | Qt::WindowSystemMenuHint | Qt::WindowTitleHint;
    case NeutralMode:
        return Qt::Window;
    default:
        return 0;
    }
}

void MainWindow::switchToNeutralMode()
{
    if ( d->mode == NeutralMode )
        return ;

    d->mode = NeutralMode;

    //FIXME hide everything...
}

void MainWindow::switchToDockedMode()
{
    if ( d->mode == DockedMode )
        return ;

    //     setUpdatesEnabled( false );

    switchToNeutralMode();

    d->mode = DockedMode;

    QList<Document* > openDocs = Core::documentController() ->openDocuments();
    QList<Document* >::const_iterator it = openDocs.begin();
    for ( ; it != openDocs.end(); ++it )
    {
        if ( !( *it ) ->isInitialized() )
            continue;

        if ( QWidget * widget = ( *it ) ->part() ->widget() )
        {
            widget->setParent( magicalParent(), magicalWindowFlags( widget ) );
            widget->setMinimumSize( 0, 0 );
            d->center->addWidget( widget );
            widget->show();
            widget->setFocus();
        }
    }

    QRegExp rx( "*_dock" );
    rx.setPatternSyntax( QRegExp::Wildcard );
    QList<QDockWidget*> dockList = qFindChildren<QDockWidget*>( this, rx );

    QList<QDockWidget*>::const_iterator it2 = dockList.constBegin();
    for ( ; it2 != dockList.constEnd(); ++it2 )
    {
        QDockWidget *dock = ( *it2 );
        QWidget *widget = magicalWidget( dock );

        if ( widget == d->centralPlugin )
        {
            d->center->removeWidget( widget );
            dock->setWidget( widget );
            widget->show();
            dock->show();
            continue;
        }

        widget->setParent( magicalParent(), magicalWindowFlags( widget ) );
        widget->setMinimumSize( 0, 0 ); //FIXME do better geometry and move the windows
        dock->setWidget( widget );

        dock->show();
    }

    //     setUpdatesEnabled( true );
}

void MainWindow::switchToTopLevelMode()
{
    if ( d->mode == TopLevelMode )
        return ;

    //     setUpdatesEnabled( false );

    switchToNeutralMode();

    d->mode = TopLevelMode;

    QList<Document* > openDocs = Core::documentController() ->openDocuments();
    QList<Document* >::const_iterator it = openDocs.begin();
    for ( ; it != openDocs.end(); ++it )
    {
        if ( !( *it ) ->isInitialized() )
            continue;

        if ( QWidget * widget = ( *it ) ->part() ->widget() )
        {
            d->center->removeWidget( widget );
            widget->setParent( magicalParent(), magicalWindowFlags( widget ) );
            widget->setMinimumSize( 640, 480 ); //FIXME do better geometry and move the windows
            widget->raise();
            widget->show();
            widget->activateWindow();
        }
    }

    QRegExp rx( "*_dock" );
    rx.setPatternSyntax( QRegExp::Wildcard );
    QList<QDockWidget*> dockList = qFindChildren<QDockWidget*>( this, rx );

    QList<QDockWidget*>::const_iterator it2 = dockList.constBegin();
    for ( ; it2 != dockList.constEnd(); ++it2 )
    {
        QDockWidget *dock = ( *it2 );
        QWidget *widget = magicalWidget( dock );

        dock->hide();

        if ( widget == d->centralPlugin )
        {
            QRect g = widget->geometry();
            d->center->addWidget( widget );
            widget->show();
            setGeometry( g );
            continue;
        }

        widget->setParent( magicalParent(), magicalWindowFlags( widget ) );
        widget->setMinimumSize( 200, 200 ); //FIXME do better geometry and move the windows
        widget->show();
    }

    //     setUpdatesEnabled( true );

    //     Qt::WindowFlags flags = windowFlags();
    //     flags |= Qt::WindowStaysOnTopHint;
    //     setWindowFlags( flags );
    //     show();
}

}

#include "kdevmainwindow.moc"
