/***************************************************************************
                          dbgcontroller.h  -  description                              
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

#ifndef __dbgcontroller_h__
#define __dbgcontroller_h__

#include <qobject.h>

class Breakpoint;
class DbgCommand;
class KProcess;
class QString;
class QStrList;
class VarItem;

/***************************************************************************/
/**
  *@author jbb
  */
/***************************************************************************/
// sigh - namespace's don't work on some of the older compilers
enum DBGStateFlags
{
  s_dbgNotStarted     = 1,
  s_appNotStarted     = 2,
  s_appBusy           = 4,
  s_waitForWrite      = 8,
  s_programExited     = 16,
  s_silent            = 32,
  s_viewLocals        = 64,
  s_viewBT            = 128,
  s_viewBP            = 256,
};
/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class DbgController : public QObject
{
  Q_OBJECT

public:

	DbgController();
	virtual ~DbgController();
  virtual void reConfig()                                                 = 0;

protected:
  virtual void queueCmd(DbgCommand* cmd, bool executeNext)                = 0;
  virtual char* parse(char* str)                                          = 0;

public slots:
  virtual void slotStart(const QString& application, const QString& args) = 0;
  virtual void slotCoreFile(const QString& coreFile)                      = 0;
  virtual void slotAttachTo(const QString& AttachTo)                      = 0;

	virtual void slotRun() 																						      = 0;
  virtual void slotRunUntil(const QString& filename, int lineNo)          = 0;
	virtual void slotStepInto()                                             = 0;
	virtual void slotStepOver()                                             = 0;
  virtual void slotStepOutOff()                                           = 0;

  virtual void slotBreakInto()                                            = 0;
  virtual void slotBPState(Breakpoint* BP)                                = 0;

  virtual void slotDisassemble(const QString& start, const QString& end)  = 0;
  virtual void slotMemoryDump(const QString& start, const QString& amount)= 0;
  virtual void slotRegisters()                                            = 0;
  virtual void slotLibraries()                                            = 0;

  virtual void slotSelectFrame(int frame)                                 = 0;

  virtual void slotExpandItem(VarItem* parent)                            = 0;
  virtual void slotExpandUserItem(VarItem* parent,
                                    const QString& userRequest)           = 0;
	
protected slots:
  virtual void slotDbgStdout(KProcess* proc, char* buf, int buflen)       = 0;
  virtual void slotDbgStderr(KProcess* proc, char* buf, int buflen) {} ;
  virtual void slotDbgWroteStdin(KProcess *proc)                          = 0;
  virtual void slotDbgProcessExited(KProcess* proc)                       = 0;

signals:
  void rawData              (const char* rawData);
	void gotoSourcePosition   (const QString& filename, int lineno);
  void rawGDBBreakpointList (char* buf);
  void rawGDBBreakpointSet  (char* buf, int key);
  void rawGDBDisassemble    (char* buf);
  void rawGDBMemoryDump     (char* buf);
  void rawGDBRegisters      (char* buf);
  void rawGDBLibraries      (char* buf);
  void ttyStdout            (const char* output);
  void ttyStderr            (const char* output);
  	
protected:
  KProcess *dbgProcess_;
};

#endif    // __dbgcontroller_h__
