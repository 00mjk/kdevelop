/***************************************************************************
                          gdbcontroller.h  -  description                              
                             -------------------                                         
    begin                : Sun Aug 8 1999                                           
    copyright            : (C) 1999 by John Birch
    email                : jb.nz@writeme.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/

#ifndef GDBCONTROLLER_H
#define GDBCONTROLLER_H

#include "dbgcontroller.h"
#include <qobject.h>
#include <qlist.h>

class Breakpoint;
class DbgCommand;
class FrameStack;
class KProcess;
class QString;
class VarItem;
class VarTree;
class STTY;

/***************************************************************************/
/**A front end implementation to the gdb command line debugger
  *@author jbb
  */
/***************************************************************************/

class GDBController : public DbgController
{
  Q_OBJECT

public:
  GDBController(VarTree* varTree, FrameStack* frameStack);
  ~GDBController();
  void reConfig();

protected:
  void queueCmd(DbgCommand* cmd, bool executeNext=false);

private:
  void parseProgramLocation (char* buf);
  void parseBacktraceList   (char* buf);
  void parseThreadList      (char* buf);
  void parseBreakpointSet   (char* buf);
  void parseLocals          (char* buf);
  void parseRequestedData   (char* buf);
  void parseLine            (char* buf);
  void parseFrameSelected   (char* buf);
  void parseQTVersion       (char* buf);

  char* parse               (char* buf);
  char* parseOther          (char* buf);
  char* parseCmdBlock       (char* buf);

  void pauseApp();
  void executeCmd ();
  void destroyCmds();
  void removeInfoRequests();
  void actOnProgramPause(const QString& msg);
  void programNoApp(const QString& msg, bool msgBox);

  void setBreakpoint(const QCString& BPSetCmd, int key);
  void clearBreakpoint(const QCString& BPClearCmd);
  void modifyBreakpoint(Breakpoint* BP);

  void setStateOn(int stateOn)    { state_ |= stateOn; }
  void setStateOff(int stateOff)  { state_ &= ~stateOff; }
  bool stateIsOn(int state)       { return state_ & state; }

public slots:
  void slotStart(const QString& application, const QString& args, const QString& sDbgShell=QString());
  void slotCoreFile(const QString& coreFile);
  void slotAttachTo(int pid);
  
  void slotRun();                                            
  void slotRunUntil(const QString& filename, int lineNo);
  void slotStepInto();
  void slotStepOver();
  void slotStepIntoIns();
  void slotStepOverIns();
  void slotStepOutOff();

  void slotBreakInto();
  void slotBPState(Breakpoint* BP);
  void slotClearAllBreakpoints();

  void slotDisassemble(const QString& start, const QString& end);
  void slotMemoryDump(const QString& start, const QString& amount);
  void slotRegisters();
  void slotLibraries();

  void slotSelectFrame(int frame, int thread, bool needFrames);

  void slotExpandItem(VarItem* parent);
  void slotExpandUserItem(VarItem* parent, const QCString& userRequest);
  void slotSetLocalViewState(bool onOff);

protected slots:
  void slotDbgStdout(KProcess* proc, char* buf, int buflen);
  void slotDbgStderr(KProcess* proc, char* buf, int buflen);
  void slotDbgWroteStdin(KProcess *proc);
  void slotDbgProcessExited(KProcess* proc);
  void slotStepInSource(const QString& filename, int lineNo);
  void slotDbgStatus(const QString& status, int state);

private slots:
  void slotAbortTimedEvent();

signals:
  void rawData              (const QString& rawData);
  void showStepInSource     (const QString& filename, int lineno, const QString& address);
  void rawGDBBreakpointList (char* buf);
  void rawGDBBreakpointSet  (char* buf, int key);
  void rawGDBDisassemble    (char* buf);
  void rawGDBMemoryDump     (char* buf);
  void rawGDBRegisters      (char* buf);
  void rawGDBLibraries      (char* buf);
  void ttyStdout            (const char* output);
  void ttyStderr            (const char* output);
  void dbgStatus            (const QString& status, int statusFlag);
  void acceptPendingBPs      ();
  void unableToSetBPNow     (int BPNo);

private:
  FrameStack*       frameStack_;
  VarTree*          varTree_;
  int               currentFrame_;
  int               viewedThread_;

  int               state_;
  int               gdbSizeofBuf_;          // size of the output buffer
  int               gdbOutputLen_;          // amount of data in the output buffer
  char*             gdbOutput_;             // buffer for the output from kprocess

  QList<DbgCommand> cmdList_;
  DbgCommand*       currentCmd_;

  STTY*             tty_;
  bool              programHasExited_;
  QString           badCore_;

  // Configuration values
  bool    config_breakOnLoadingLibrary_;
  bool    config_forceBPSet_;
  bool    config_displayStaticMembers_;
  bool    config_asmDemangle_;
  bool    config_dbgTerminal_;
  QString config_gdbPath_;
};

#endif
