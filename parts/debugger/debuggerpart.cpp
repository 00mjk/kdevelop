/***************************************************************************
 *   Copyright (C) 1999-2001 by John Birch                                 *
 *   jbb@kdevelop.org                                                      *
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <qdir.h>
#include <qvbox.h>
#include <qwhatsthis.h>
#include <qpopupmenu.h>

#include <kaction.h>
#include <kdebug.h>
#include <kfiledialog.h>
#include <kgenericfactory.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kmainwindow.h>
#include <kstatusbar.h>
#include <kparts/part.h>
#include <ktexteditor/viewcursorinterface.h>
#include <kmessagebox.h>
#include <kapplication.h>
#include <dcopclient.h>
#include <qtimer.h>

#include "kdevcore.h"
#include "kdevproject.h"
#include "kdevmainwindow.h"
#include "kdevappfrontend.h"
#include "kdevpartcontroller.h"
#include "kdevdebugger.h"
#include "domutil.h"
#include "variablewidget.h"
#include "gdbbreakpointwidget.h"
#include "framestackwidget.h"
#include "disassemblewidget.h"
#include "processwidget.h"
#include "gdbcontroller.h"
#include "breakpoint.h"
#include "dbgpsdlg.h"
#include "dbgtoolbar.h"
#include "memviewdlg.h"
#include "gdbparser.h"
#include "gdboutputwidget.h"
#include "debuggerconfigwidget.h"
#include "processlinemaker.h"

#include <iostream>

#include "debuggerpart.h"

namespace GDBDebugger
{

typedef KGenericFactory<DebuggerPart> DebuggerFactory;
K_EXPORT_COMPONENT_FACTORY( libkdevdebugger, DebuggerFactory( "kdevdebugger" ) );

DebuggerPart::DebuggerPart( QObject *parent, const char *name, const QStringList & ) :
    KDevPlugin( "CppDebugger", "debugger", parent, name ? name : "DebuggerPart" ),
    controller(0)
{
    setObjId("DebuggerInterface");
    setInstance(DebuggerFactory::instance());

    setXMLFile("kdevdebugger.rc");

    statusBarIndicator = new QLabel(" ", mainWindow()->statusBar());
    statusBarIndicator->setFixedWidth(15);
    mainWindow()->statusBar()->addWidget(statusBarIndicator, 0, true);
    statusBarIndicator->show();

    // Setup widgets and dbgcontroller
    variableWidget = new VariableWidget();
    variableWidget->setEnabled(false);
    variableWidget->setIcon(SmallIcon("math_brace"));
    variableWidget->setCaption(i18n("Variable Tree"));
    QWhatsThis::add
        (variableWidget, i18n("Variable tree\n\n"
                              "The variable tree allows you to see "
                              "the variable values as you step "
                              "through your program using the internal "
                              "debugger. Click the RMB on items in "
                              "this view to get a popup menu.\n"
                              "To speed up stepping through your code "
                              "leave the tree items closed and add the "
                              "variable(s) to the watch section.\n"
                              "To change a variable value in your "
                              "running app use a watch variable (eg a=5)."));
    mainWindow()->embedSelectView(variableWidget, i18n("Variables / Watch"), i18n("debugger variable-view"));
    mainWindow()->setViewAvailable(variableWidget, false);

    gdbBreakpointWidget = new GDBBreakpointWidget();
    gdbBreakpointWidget->setCaption(i18n("Breakpoint List"));
    QWhatsThis::add
        (gdbBreakpointWidget, i18n("Breakpoint list\n\n"
                                "Displays a list of breakpoints with "
                                "their current status. Clicking on a "
                                "breakpoint item allows you to change "
                                "the breakpoint and will take you "
                                "to the source in the editor window."));
    mainWindow()->embedOutputView(gdbBreakpointWidget, i18n("Breakpoints"), i18n("debugger breakpoints"));

    framestackWidget = new FramestackWidget();
    framestackWidget->setEnabled(false);
    framestackWidget->setCaption(i18n("Frame Stack"));
    QWhatsThis::add
        (framestackWidget, i18n("Frame stack\n\n"
                                "Often referred to as the \"call stack\", "
                                "this is a list showing what function is "
                                "currently active and who called each "
                                "function to get to this point in your "
                                "program. By clicking on an item you "
                                "can see the values in any of the "
                                "previous calling functions."));
    mainWindow()->embedOutputView(framestackWidget, i18n("Frame Stack"), i18n("debugger function call stack"));
    mainWindow()->setViewAvailable(framestackWidget, false);

    disassembleWidget = new DisassembleWidget();
    disassembleWidget->setEnabled(false);
    disassembleWidget->setCaption(i18n("Machine Code Display"));
    QWhatsThis::add
        (disassembleWidget, i18n("Machine code display\n\n"
                                 "A machine code view into your running "
                                 "executable with the current instruction "
                                 "highlighted. You can step instruction by "
                                 "instruction using the debuggers toolbar "
                                 "buttons of \"step over\" instruction and "
                                 "\"step into\" instruction."));
    mainWindow()->embedOutputView(disassembleWidget, i18n("Disassemble"),
                                  i18n("debugger disassemble view"));
    mainWindow()->setViewAvailable(disassembleWidget, false);

    gdbOutputWidget = new GDBOutputWidget;
    gdbOutputWidget->setEnabled(false);
    mainWindow()->embedOutputView(gdbOutputWidget, i18n("GDB"),
                                  i18n("GDB output"));
    mainWindow()->setViewAvailable(gdbOutputWidget, false);

    VariableTree *variableTree = variableWidget->varTree();

    // variableTree -> framestackWidget
    connect( variableTree,     SIGNAL(selectFrame(int, int)),
             framestackWidget, SLOT(slotSelectFrame(int, int)));

    // gdbBreakpointWidget -> this
    connect( gdbBreakpointWidget, SIGNAL(refreshBPState(const Breakpoint&)),
             this,             SLOT(slotRefreshBPState(const Breakpoint&)));
    connect( gdbBreakpointWidget, SIGNAL(publishBPState(const Breakpoint&)),
             this,             SLOT(slotRefreshBPState(const Breakpoint&)));
    connect( gdbBreakpointWidget, SIGNAL(gotoSourcePosition(const QString&, int)),
             this,             SLOT(slotGotoSource(const QString&, int)) );

    // Now setup the actions
    KAction *action;

    action = new KAction(i18n("&Start"), "1rightarrow", 0,
                         this, SLOT(slotRun()),
                         actionCollection(), "debug_run");
    action->setStatusText( i18n("Runs the program in the debugger") );
    action->setWhatsThis( i18n("Start in debugger\n\n"
                               "Starts the debugger with the project's main "
                               "executable. You may set some breakpoints "
                               "before this, or you can interrupt the program "
                               "while it is running, in order to get information "
                               "about variables, frame stack, and so on.") );

    action = new KAction(i18n("Sto&p"), "stop", 0,
                         this, SLOT(slotStop()),
                         actionCollection(), "debug_stop");
    action->setStatusText( i18n("Kills the executable and exits the debugger") );

    action = new KAction(i18n("Interrupt"), "player_pause", 0,
                         this, SLOT(slotPause()),
                         actionCollection(), "debug_pause");
    action->setStatusText( i18n("Interrupts the application") );

    action = new KAction(i18n("Run to &Cursor"), "dbgrunto", 0,
                         this, SLOT(slotRunToCursor()),
                         actionCollection(), "debug_runtocursor");
    action->setStatusText( i18n("Continues execution until the cursor position is reached") );


    action = new KAction(i18n("Step &Over"), "dbgnext", 0,
                         this, SLOT(slotStepOver()),
                         actionCollection(), "debug_stepover");
    action->setStatusText( i18n("Steps over the next line") );
    action->setWhatsThis( i18n("Step over\n\n"
                               "Executes one line of source in the current source file. "
                               "If the source line is a call to a function the whole "
                               "function is executed and the app will stop at the line "
                               "following the function call.") );


    action = new KAction(i18n("Step over Ins&truction"), "dbgnextinst", 0,
                         this, SLOT(slotStepOverInstruction()),
                         actionCollection(), "debug_stepoverinst");
    action->setStatusText( i18n("Steps over the next assembly instruction") );


    action = new KAction(i18n("Step &Into"), "dbgstep", 0,
                         this, SLOT(slotStepInto()),
                         actionCollection(), "debug_stepinto");
    action->setStatusText( i18n("Steps into the next statement") );
    action->setWhatsThis( i18n("Step into\n\n"
                               "Executes exactly one line of source. If the source line "
                               "is a call to a function then execution will stop after "
                               "the function has been entered.") );


    action = new KAction(i18n("Step into I&nstruction"), "dbgstepinst", 0,
                         this, SLOT(slotStepIntoInstruction()),
                         actionCollection(), "debug_stepintoinst");
    action->setStatusText( i18n("Steps into the next assembly instruction") );


    action = new KAction(i18n("Step O&ut"), "dbgstepout", 0,
                         this, SLOT(slotStepOut()),
                         actionCollection(), "debug_stepout");
    action->setStatusText( i18n("Steps out of the current function") );
    action->setWhatsThis( i18n("Step out of\n\n"
                               "Executes the application until the currently executing "
                               "function is completed. The debugger will then display "
                               "the line after the original call to that function. If "
                               "program execution is in the outermost frame (i.e. in "
                               "main()) then this operation has no effect.") );


    action = new KAction(i18n("Viewers"), "dbgmemview", 0,
                         this, SLOT(slotMemoryView()),
                         actionCollection(), "debug_memview");
    action->setStatusText( i18n("Various views into the application") );


    action = new KAction(i18n("Examine Core File"), "core", 0,
                         this, SLOT(slotExamineCore()),
                         actionCollection(), "debug_core");
    action->setStatusText( i18n("Loads a core file into the debugger") );
    action->setWhatsThis( i18n("Examine core file\n\n"
                               "This loads a core file, which is typically created "
                               "after the application has crashed, e.g. with a "
                               "segmentation fault. The core file contains an "
                               "image of the program memory at the time it crashed, "
                               "allowing you to do a post-mortem analysis.") );


    action = new KAction(i18n("Attach to Process"), "connect_creating", 0,
                         this, SLOT(slotAttachProcess()),
                         actionCollection(), "debug_attach");
    action->setStatusText( i18n("Attaches the debugger to a running process") );


    action = new KAction(i18n("Toggle Breakpoint"), 0, 0,
                         this, SLOT(toggleBreakpoint()),
                         actionCollection(), "debug_toggle_breakpoint");

    connect( mainWindow()->main()->guiFactory(), SIGNAL(clientAdded(KXMLGUIClient*)),
             this, SLOT(guiClientAdded(KXMLGUIClient*)) );

    connect( core(), SIGNAL(projectConfigWidget(KDialogBase*)),
             this, SLOT(projectConfigWidget(KDialogBase*)) );

    connect( partController(), SIGNAL(loadedFile(const QString &)),
             gdbBreakpointWidget, SLOT(slotRefreshBP(const QString &)) );
    connect( debugger(), SIGNAL(toggledBreakpoint(const QString &, int)),
             gdbBreakpointWidget, SLOT(slotToggleBreakpoint(const QString &, int)) );
    connect( debugger(), SIGNAL(editedBreakpoint(const QString &, int)),
             gdbBreakpointWidget, SLOT(slotEditBreakpoint(const QString &, int)) );
    connect( debugger(), SIGNAL(toggledBreakpointEnabled(const QString &, int)),
             gdbBreakpointWidget, SLOT(slotToggleBreakpointEnabled(const QString &, int)) );

    connect( core(), SIGNAL(contextMenu(QPopupMenu *, const Context *)),
             this, SLOT(contextMenu(QPopupMenu *, const Context *)) );

    connect( core(), SIGNAL(stopButtonClicked(KDevPlugin*)),
             this, SLOT(slotStop(KDevPlugin*)) );
    connect( core(), SIGNAL(projectClosed()),
             this, SLOT(projectClosed()) );

    connect( partController(), SIGNAL(activePartChanged(KParts::Part*)),
             this, SLOT(slotActivePartChanged(KParts::Part*)) );

    procLineMaker = new ProcessLineMaker();

    connect( procLineMaker, SIGNAL(receivedStdoutLine(const QString&)),
             appFrontend(), SLOT(insertStdoutLine(const QString&)) );
    connect( procLineMaker, SIGNAL(receivedStderrLine(const QString&)),
             appFrontend(), SLOT(insertStderrLine(const QString&)) );

    setupController();

    QCStringList objects = kapp->dcopClient()->registeredApplications();
    for (QCStringList::Iterator it = objects.begin(); it != objects.end(); ++it)
        if ((*it).find("drkonqi-") == 0)
            slotDCOPApplicationRegistered(*it);

    connect(kapp->dcopClient(), SIGNAL(applicationRegistered(const QCString&)), SLOT(slotDCOPApplicationRegistered(const QCString&)));
    kapp->dcopClient()->setNotifications(true);
}

void DebuggerPart::slotDCOPApplicationRegistered(const QCString& appId)
{
    if (appId.find("drkonqi-") == 0) {
        QByteArray answer;
        QCString replyType;

#if defined(KDE_MAKE_VERSION)
# if KDE_VERSION >= KDE_MAKE_VERSION(3,1,90)
        kapp->dcopClient()->call(appId, "krashinfo", "appName()", QByteArray(), replyType, answer, true, 5000);
# else
        kapp->dcopClient()->call(appId, "krashinfo", "appName()", QByteArray(), replyType, answer, true);
# endif
#else
        kapp->dcopClient()->call(appId, "krashinfo", "appName()", QByteArray(), replyType, answer, true);
#endif

        QDataStream d(answer, IO_ReadOnly);
        QCString appName;
        d >> appName;

        if (appName.length() && project() && project()->mainProgram().endsWith(appName)) {
            kapp->dcopClient()->send(appId, "krashinfo", "registerDebuggingApplication(QString)", i18n("Debug in &KDevelop"));
            connectDCOPSignal(appId, "krashinfo", "acceptDebuggingApplication()", "slotDebugExternalProcess()", true);
        }
    }
}

ASYNC DebuggerPart::slotDebugExternalProcess()
{
    QByteArray answer;
    QCString replyType;

#if defined(KDE_MAKE_VERSION)
# if KDE_VERSION >= KDE_MAKE_VERSION(3,1,90)
    kapp->dcopClient()->call(kapp->dcopClient()->senderId(), "krashinfo", "pid()", QByteArray(), replyType, answer, true, 5000);
# else
    kapp->dcopClient()->call(kapp->dcopClient()->senderId(), "krashinfo", "pid()", QByteArray(), replyType, answer, true);
# endif
#else
    kapp->dcopClient()->call(kapp->dcopClient()->senderId(), "krashinfo", "pid()", QByteArray(), replyType, answer, true);
#endif

    QDataStream d(answer, IO_ReadOnly);
    int pid;
    d >> pid;

    if (attachProcess(pid) && m_drkonqi.isEmpty()) {
        m_drkonqi = kapp->dcopClient()->senderId();
        QTimer::singleShot(15000, this, SLOT(slotCloseDrKonqi()));
        mainWindow()->raiseView(framestackWidget);
    }

    mainWindow()->main()->raise();
}

void DebuggerPart::slotCloseDrKonqi()
{
    kapp->dcopClient()->send(m_drkonqi, "MainApplication-Interface", "quit()", QByteArray());
    m_drkonqi = "";
}

DebuggerPart::~DebuggerPart()
{
    kapp->dcopClient()->setNotifications(false);

    if (variableWidget)
        mainWindow()->removeView(variableWidget);
    if (gdbBreakpointWidget)
        mainWindow()->removeView(gdbBreakpointWidget);
    if (framestackWidget)
        mainWindow()->removeView(framestackWidget);
    if (disassembleWidget)
        mainWindow()->removeView(disassembleWidget);
    if(gdbOutputWidget)
        mainWindow()->removeView(gdbOutputWidget);

    delete variableWidget;
    delete gdbBreakpointWidget;
    delete framestackWidget;
    delete disassembleWidget;
    delete gdbOutputWidget;
    delete controller;
    delete floatingToolBar;
    delete statusBarIndicator;
    delete procLineMaker;

    GDBParser::destroy();
}


void DebuggerPart::guiClientAdded( KXMLGUIClient* client )
{
    // Can't change state until after XMLGUI has been loaded...
    // Anyone know of a better way of doing this?
    if( client == this )
        stateChanged( QString("stopped") );
}

void DebuggerPart::contextMenu(QPopupMenu *popup, const Context *context)
{
    if (!context->hasType( Context::EditorContext ))
        return;

    const EditorContext *econtext = static_cast<const EditorContext*>(context);
    m_contextIdent = econtext->currentWord();

    popup->insertSeparator();
    if (econtext->url().isLocalFile())
        popup->insertItem( i18n("Toggle Breakpoint"), this, SLOT(toggleBreakpoint()) );
    if (!m_contextIdent.isEmpty())
        popup->insertItem( i18n("Watch: %1").arg(m_contextIdent), this, SLOT(contextWatch()) );
}


void DebuggerPart::toggleBreakpoint()
{
    KParts::ReadWritePart *rwpart
        = dynamic_cast<KParts::ReadWritePart*>(partController()->activePart());
    KTextEditor::ViewCursorInterface *cursorIface
        = dynamic_cast<KTextEditor::ViewCursorInterface*>(partController()->activeWidget());

    if (!rwpart || !cursorIface)
        return;

    uint line, col;
    cursorIface->cursorPositionReal(&line, &col);

    gdbBreakpointWidget->slotToggleBreakpoint(rwpart->url().path(), line);
}


void DebuggerPart::contextWatch()
{
    variableWidget->slotAddWatchVariable(m_contextIdent);
}


void DebuggerPart::projectConfigWidget(KDialogBase *dlg)
{
    QVBox *vbox = dlg->addVBoxPage(i18n("Debugger"));
    DebuggerConfigWidget *w = new DebuggerConfigWidget(this, vbox, "debugger config widget");
    connect( dlg, SIGNAL(okClicked()), w, SLOT(accept()) );
    connect( dlg, SIGNAL(finished()), controller, SLOT(configure()) );
}


void DebuggerPart::setupController()
{
    VariableTree *variableTree = variableWidget->varTree();

    controller = new GDBController(variableTree, framestackWidget, *projectDom());

    // variableTree -> controller
    connect( variableTree,          SIGNAL(expandItem(TrimmableItem*)),
             controller,            SLOT(slotExpandItem(TrimmableItem*)));
    connect( variableTree,          SIGNAL(expandUserItem(VarItem*, const QCString&)),
             controller,            SLOT(slotExpandUserItem(VarItem*, const QCString&)));
    connect( variableTree,          SIGNAL(setLocalViewState(bool)),
             controller,            SLOT(slotSetLocalViewState(bool)));
    connect( variableTree,          SIGNAL(varItemConstructed(VarItem*)),
             controller,            SLOT(slotVarItemConstructed(VarItem*)));     // jw

    // variableTree -> gdbBreakpointWidget
    connect( variableTree,          SIGNAL(toggleWatchpoint(const QString &)),
             gdbBreakpointWidget,   SLOT(slotToggleWatchpoint(const QString &)));

    // framestackWidget -> controller
    connect( framestackWidget,      SIGNAL(selectFrame(int,int,bool)),
             controller,            SLOT(slotSelectFrame(int,int,bool)));

    // gdbBreakpointWidget -> controller
    connect( gdbBreakpointWidget,   SIGNAL(clearAllBreakpoints()),
             controller,            SLOT(slotClearAllBreakpoints()));
    connect( gdbBreakpointWidget,   SIGNAL(publishBPState(const Breakpoint&)),
             controller,            SLOT(slotBPState(const Breakpoint &)));

    // disassembleWidget -> controller
    connect( disassembleWidget,     SIGNAL(disassemble(const QString&, const QString&)),
             controller,            SLOT(slotDisassemble(const QString&, const QString&)));

    // gdbOutputWidget -> controller
    connect( gdbOutputWidget,       SIGNAL(userGDBCmd(const QString &)),
             controller,            SLOT(slotUserGDBCmd(const QString&)));
    connect( gdbOutputWidget,       SIGNAL(breakInto()),
             controller,            SLOT(slotBreakInto()));

    // controller -> gdbBreakpointWidget
    connect( controller,            SIGNAL(acceptPendingBPs()),
             gdbBreakpointWidget,   SLOT(slotSetPendingBPs()));
    connect( controller,            SIGNAL(unableToSetBPNow(int)),
             gdbBreakpointWidget,   SLOT(slotUnableToSetBPNow(int)));
    connect( controller,            SIGNAL(rawGDBBreakpointList (char*)),
             gdbBreakpointWidget,   SLOT(slotParseGDBBrkptList(char*)));
    connect( controller,            SIGNAL(rawGDBBreakpointSet(char*, int)),
             gdbBreakpointWidget,   SLOT(slotParseGDBBreakpointSet(char*, int)));

    // controller -> disassembleWidget
    connect( controller,            SIGNAL(showStepInSource(const QString&, int, const QString&)),
             disassembleWidget,     SLOT(slotShowStepInSource(const QString&, int, const QString&)));
    connect( controller,            SIGNAL(rawGDBDisassemble(char*)),
             disassembleWidget,     SLOT(slotDisassemble(char*)));

    // controller -> this
    connect( controller,            SIGNAL(dbgStatus(const QString&, int)),
             this,                  SLOT(slotStatus(const QString&, int)));
    connect( controller,            SIGNAL(showStepInSource(const QString&, int, const QString&)),
             this,                  SLOT(slotShowStep(const QString&, int)));

    // controller -> procLineMaker
    connect( controller,            SIGNAL(ttyStdout(const char*)),
             procLineMaker,         SLOT(slotReceivedStdout(const char*)));
    connect( controller,            SIGNAL(ttyStderr(const char*)),
             procLineMaker,         SLOT(slotReceivedStderr(const char*)));

    // controller -> gdbOutputWidget
    connect( controller,            SIGNAL(gdbStdout(const char*)),
             gdbOutputWidget,       SLOT(slotReceivedStdout(const char*)) );
    connect( controller,            SIGNAL(gdbStderr(const char*)),
             gdbOutputWidget,       SLOT(slotReceivedStderr(const char*)) );
    connect( controller,            SIGNAL(dbgStatus(const QString&, int)),
             gdbOutputWidget,       SLOT(slotDbgStatus(const QString&, int)));

    // gdbBreakpointWidget -> disassembleWidget
    connect( gdbBreakpointWidget,   SIGNAL(publishBPState(const Breakpoint&)),
             disassembleWidget,     SLOT(slotBPState(const Breakpoint &)));
}


bool DebuggerPart::startDebugger()
{
    QString build_dir;              // Currently selected build directory
    DomUtil::PairList run_envvars;  // List with the environment variables
    QString run_directory;          // Directory from where the program should be run
    QString program;                // Absolute path to application
    QString run_arguments;          // Command line arguments to be passed to the application

    if (project()) {
        build_dir     = project()->buildDirectory();
        run_envvars   = project()->runEnvironmentVars();
        run_directory = project()->runDirectory();
        program       = project()->mainProgram();
        run_arguments = project()->runArguments();
    }

    QString shell = DomUtil::readEntry(*projectDom(), "/kdevdebugger/general/dbgshell");
    if( !shell.isEmpty() )
    {
        QFileInfo info( shell );
        if( info.isRelative() )
        {
            shell = build_dir + "/" + shell;
            info.setFile( shell );
        }
        if( !info.exists() )
        {
            KMessageBox::error(
                mainWindow()->main(),
                i18n("Could not locate the debugging shell '%1'.").arg( shell ),
                i18n("Debugging shell not found.") );
            return false;
        }
    }

    core()->running(this, true);

    stateChanged( QString("active") );

    KActionCollection *ac = actionCollection();
    ac->action("debug_run")->setText( i18n("&Continue") );
    ac->action("debug_run")->setIcon( "dbgrun" );
    ac->action("debug_run")->setStatusText( i18n("Continues the application execution") );
    ac->action("debug_run")->setWhatsThis( i18n("Continue application execution\n\n"
                                           "Continues the execution of your application in the "
                                           "debugger. This only takes effect when the application "
                                           "has been halted by the debugger (i.e. a breakpoint has "
                                           "been activated or the interrupt was pressed).") );


    mainWindow()->setViewAvailable(variableWidget, true);
    mainWindow()->setViewAvailable(framestackWidget, true);
    mainWindow()->setViewAvailable(disassembleWidget, true);
    mainWindow()->setViewAvailable(gdbOutputWidget, true);

    variableWidget->setEnabled(true);
    framestackWidget->setEnabled(true);
    disassembleWidget->setEnabled(true);

    gdbOutputWidget->clear();
    gdbOutputWidget->setEnabled(true);

    if (DomUtil::readBoolEntry(*projectDom(), "/kdevdebugger/general/floatingtoolbar", true))
    {
        floatingToolBar = new DbgToolBar(this, mainWindow()->main());
        floatingToolBar->show();
    }

    controller->slotStart(shell, run_envvars, run_directory, program, run_arguments);
    return true;
}

void DebuggerPart::slotStopDebugger()
{
    controller->slotStopDebugger();
    debugger()->clearExecutionPoint();

    delete floatingToolBar;
    floatingToolBar = 0;

    gdbBreakpointWidget->reset();
    framestackWidget->clear();
    variableWidget->clear();
    disassembleWidget->clear();
    disassembleWidget->slotActivate(false);

    variableWidget->setEnabled(false);
    framestackWidget->setEnabled(false);
    disassembleWidget->setEnabled(false);
    gdbOutputWidget->setEnabled(false);

    mainWindow()->setViewAvailable(variableWidget, false);
    mainWindow()->setViewAvailable(framestackWidget, false);
    mainWindow()->setViewAvailable(disassembleWidget, false);
    mainWindow()->setViewAvailable(gdbOutputWidget, false);

    KActionCollection *ac = actionCollection();
    ac->action("debug_run")->setText( i18n("&Start") );
    ac->action("debug_run")->setIcon( "1rightarrow" );
    ac->action("debug_run")->setStatusText( i18n("Runs the program in the debugger") );
    ac->action("debug_run")->setWhatsThis( i18n("Start in debugger\n\n"
                                           "Starts the debugger with the project's main "
                                           "executable. You may set some breakpoints "
                                           "before this, or you can interrupt the program "
                                           "while it is running, in order to get information "
                                           "about variables, frame stack, and so on.") );

    stateChanged( QString("stopped") );

    core()->running(this, false);
}

void DebuggerPart::projectClosed()
{
    slotStopDebugger();
}

void DebuggerPart::slotRun()
{
    if( controller->stateIsOn( s_dbgNotStarted ) )
    {
        mainWindow()->statusBar()->message(i18n("Debugging program"), 1000);
        mainWindow()->raiseView(gdbOutputWidget);
        startDebugger();
    }
    else
    {
        mainWindow()->statusBar()->message(i18n("Continuing program"), 1000);
    }
    controller->slotRun();
}


void DebuggerPart::slotExamineCore()
{
    mainWindow()->statusBar()->message(i18n("Choose a core file to examine..."), 1000);

    QString dirName = project()? project()->projectDirectory() : QDir::homeDirPath();
    QString coreFile = KFileDialog::getOpenFileName(dirName);
    if (coreFile.isNull())
        return;

    mainWindow()->statusBar()->message(i18n("Examining core file %1").arg(coreFile), 1000);

    startDebugger();
    controller->slotCoreFile(coreFile);
}


void DebuggerPart::slotAttachProcess()
{
    mainWindow()->statusBar()->message(i18n("Choose a process to attach to..."), 1000);

    Dbg_PS_Dialog dlg;
    if (!dlg.exec() || !dlg.pidSelected())
        return;

    int pid = dlg.pidSelected();
    attachProcess(pid);
}

bool DebuggerPart::attachProcess(int pid)
{
    mainWindow()->statusBar()->message(i18n("Attaching to process %1").arg(pid), 1000);

    bool ret = startDebugger();
    controller->slotAttachTo(pid);
    return ret;
}


void DebuggerPart::slotStop(KDevPlugin* which)
{
    if( which != 0 && which != this )
        return;

//    if( !controller->stateIsOn( s_dbgNotStarted ) && !controller->stateIsOn( s_shuttingDown ) )
        slotStopDebugger();
}


void DebuggerPart::slotPause()
{
    controller->slotBreakInto();
}


void DebuggerPart::slotRunToCursor()
{
    KParts::ReadWritePart *rwpart
        = dynamic_cast<KParts::ReadWritePart*>(partController()->activePart());
    KTextEditor::ViewCursorInterface *cursorIface
        = dynamic_cast<KTextEditor::ViewCursorInterface*>(partController()->activeWidget());

    if (!rwpart || !rwpart->url().isLocalFile() || !cursorIface)
        return;

    uint line, col;
    cursorIface->cursorPosition(&line, &col);

    controller->slotRunUntil(rwpart->url().path(), line);
}

void DebuggerPart::slotStepOver()
{
    controller->slotStepOver();
}


void DebuggerPart::slotStepOverInstruction()
{
    controller->slotStepOver();
}


void DebuggerPart::slotStepIntoInstruction()
{
    controller->slotStepIntoIns();
}


void DebuggerPart::slotStepInto()
{
    controller->slotStepInto();
}


void DebuggerPart::slotStepOut()
{
    controller->slotStepOutOff();
}


void DebuggerPart::slotMemoryView()
{
    // Hmm, couldn't this be made non-modal?

    MemoryViewDialog *dlg = new MemoryViewDialog();
    connect( dlg,        SIGNAL(disassemble(const QString&, const QString&)),
             controller, SLOT(slotDisassemble(const QString&, const QString&)));
    connect( dlg,        SIGNAL(memoryDump(const QString&, const QString&)),
             controller, SLOT(slotMemoryDump(const QString&, const QString&)));
    connect( dlg,        SIGNAL(registers()),
             controller, SLOT(slotRegisters()));
    connect( dlg,        SIGNAL(libraries()),
             controller, SLOT(slotLibraries()));

    connect( controller, SIGNAL(rawGDBMemoryDump(char*)),
             dlg,        SLOT(slotRawGDBMemoryView(char*)));
    connect( controller, SIGNAL(rawGDBDisassemble(char*)),
             dlg,        SLOT(slotRawGDBMemoryView(char*)));
    connect( controller, SIGNAL(rawGDBRegisters(char*)),
             dlg,        SLOT(slotRawGDBMemoryView(char*)));
    connect( controller, SIGNAL(rawGDBLibraries(char*)),
             dlg,        SLOT(slotRawGDBMemoryView(char*)));

    dlg->exec();
    delete dlg;
}


void DebuggerPart::slotRefreshBPState( const Breakpoint& BP)
{
    if (BP.type() == BP_TYPE_FilePos)
    {
        const FilePosBreakpoint& bp = dynamic_cast<const FilePosBreakpoint&>(BP);
        if (bp.isActionDie())
            debugger()->setBreakpoint(bp.fileName(), bp.lineNum()-1, -1, true, false);
        else
            debugger()->setBreakpoint(bp.fileName(), bp.lineNum()-1,
                                  1/*bp->id()*/, bp.isEnabled(), bp.isPending() );
    }
}


void DebuggerPart::slotStatus(const QString &msg, int state)
{
    QString stateIndicator;

    if (state & s_dbgNotStarted)
    {
        stateIndicator = " ";
    }
    else if (state & s_appBusy)
    {
        stateIndicator = "A";
        debugger()->clearExecutionPoint();
        stateChanged( QString("active") );
    }
    else if (state & s_programExited)
    {
        stateIndicator = "E";
        slotStop();
    }
    else
    {
        stateIndicator = "P";
        stateChanged( QString("paused") );
    }

    // And now? :-)
    kdDebug(9012) << "Debugger state: " << stateIndicator << ": " << endl;
    kdDebug(9012) << "   " << msg << endl;

    statusBarIndicator->setText(stateIndicator);
    if (!msg.isEmpty())
        mainWindow()->statusBar()->message(msg, 3000);
}


void DebuggerPart::slotShowStep(const QString &fileName, int lineNum)
{
    if ( ! fileName.isEmpty() )
    {
        // Debugger counts lines from 1
        debugger()->gotoExecutionPoint(fileName, lineNum-1);
    }
}


void DebuggerPart::slotGotoSource(const QString &fileName, int lineNum)
{
    if ( ! fileName.isEmpty() )
        partController()->editDocument(fileName, lineNum);
}


void DebuggerPart::slotActivePartChanged( KParts::Part* part )
{
    KAction* action = actionCollection()->action("debug_toggle_breakpoint");
    if(!action)
        return;

    if(!part)
    {
        action->setEnabled(false);
        return;
    }
    KTextEditor::ViewCursorInterface *iface
        = dynamic_cast<KTextEditor::ViewCursorInterface*>(part->widget());
    action->setEnabled( iface != 0 );
}

void DebuggerPart::restorePartialProjectSession(const QDomElement* el)
{
    gdbBreakpointWidget->restorePartialProjectSession(el);
}

void DebuggerPart::savePartialProjectSession(QDomElement* el)
{
    gdbBreakpointWidget->savePartialProjectSession(el);
}

}

#include "debuggerpart.moc"
