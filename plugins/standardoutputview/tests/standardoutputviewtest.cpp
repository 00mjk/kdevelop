/*
    Copyright (C) 2011  Silvère Lestang <silvere.lestang@gmail.com>

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

#include <QTest>
#include <QtGui>
#include <QSignalSpy>

#include <qtest_kde.h>
#include <kdebug.h>

#include <tests/testcore.h>
#include <tests/autotestshell.h>
#include <sublime/tooldocument.h>
#include <sublime/view.h>
#include <sublime/area.h>
#include <sublime/controller.h>
#include <sublime/mainwindow.h>

#include "standardoutputviewtest.h"
#include "../standardoutputview.h"
#include "../outputwidget.h"
#include <interfaces/iplugincontroller.h>

namespace KDevelop
{
    class IUiController;
}

const QString StandardOutputViewTest::toolviewTitle = "my_toolview";

void StandardOutputViewTest::initTestCase()
{
    KDevelop::AutoTestShell::init();
    m_testCore = new KDevelop::TestCore();
    QSignalSpy spy(m_testCore, SIGNAL(initializationCompleted()));
    m_testCore->initialize(KDevelop::Core::Default);
    QCOMPARE(spy.count(), 1); // make sure the signal was emitted exactly one time
    
    m_controller = new KDevelop::UiController(m_testCore);
    m_area = new Sublime::Area( m_controller, "Area" );
    Sublime::MainWindow* mw = new Sublime::MainWindow(m_controller);
    m_controller->showArea(m_area, mw);
    
    QTest::qWait(500);
    
    m_stdOutputView = 0;
    KDevelop::IPluginController* plugin_controller = m_testCore->pluginController();
    
    QList<KDevelop::IPlugin*> plugins = plugin_controller->loadedPlugins();
    foreach(KDevelop::IPlugin* plugin, plugins) {
        if(plugin_controller->pluginInfo(plugin).pluginName() == "KDevStandardOutputView")
           m_stdOutputView =  dynamic_cast<StandardOutputView*>(plugin);
    }
    Q_ASSERT(m_stdOutputView);
}

void StandardOutputViewTest::cleanupTestCase()
{
     m_testCore->cleanup();
     delete m_testCore;
    //delete m_area;
}

OutputWidget* StandardOutputViewTest::toolviewPointer(QString toolviewTitle)
{
    QList< Sublime::View* > views = m_controller->activeArea()->toolViews();
    foreach(Sublime::View* view, views) {
        Sublime::ToolDocument *doc = dynamic_cast<Sublime::ToolDocument*>(view->document());
        if(doc)
        {
            if(doc->title() == toolviewTitle) {
                return dynamic_cast<OutputWidget*>(view->widget());
            }
        }
    }
    return 0;
}

void StandardOutputViewTest::testRegisterAndRemoveToolView()
{    
    toolviewId = m_stdOutputView->registerToolView(toolviewTitle, KDevelop::IOutputView::HistoryView);
    QVERIFY(toolviewPointer(toolviewTitle));
    
    m_stdOutputView->removeToolView(toolviewId);
    QVERIFY(!toolviewPointer(toolviewTitle));
}

void StandardOutputViewTest::testActions()
{
    toolviewId = m_stdOutputView->registerToolView(toolviewTitle, KDevelop::IOutputView::MultipleView, KIcon());
    OutputWidget* outputWidget = toolviewPointer(toolviewTitle);
    QVERIFY(outputWidget);
    
    QList<QAction*> actions = outputWidget->actions();
    QCOMPARE(actions.takeFirst()->text(), QString(""));
    QCOMPARE(actions.takeFirst()->text(), QString("Select &All"));
    QCOMPARE(actions.takeFirst()->text(), QString("&Copy"));
    
    m_stdOutputView->removeToolView(toolviewId);
    QVERIFY(!toolviewPointer(toolviewTitle));
    
    toolviewId = m_stdOutputView->registerToolView(toolviewTitle, KDevelop::IOutputView::HistoryView, 
                                                   KIcon(), KDevelop::IOutputView::ShowItemsButton | KDevelop::IOutputView::AddFilterAction);
    outputWidget = toolviewPointer(toolviewTitle);
    QVERIFY(outputWidget);
    
    actions = outputWidget->actions();
    QCOMPARE(actions.takeFirst()->text(), QString("Previous"));
    QCOMPARE(actions.takeFirst()->text(), QString("Next"));
    QCOMPARE(actions.takeFirst()->text(), QString("Select activated Item"));
    QCOMPARE(actions.takeFirst()->text(), QString("Focus when selecting Item"));
    //QCOMPARE(actions.takeFirst()->text(), QString(""));
    QCOMPARE(actions.takeFirst()->text(), QString("Select &All"));
    QCOMPARE(actions.takeFirst()->text(), QString("&Copy"));
    QCOMPARE(actions.takeFirst()->text(), QString(""));
    QCOMPARE(actions.takeFirst()->text(), QString("Filter"));
    
    m_stdOutputView->removeToolView(toolviewId);
    QVERIFY(!toolviewPointer(toolviewTitle));
}

QTEST_KDEMAIN(StandardOutputViewTest, GUI)