/***************************************************************************
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

#ifndef _GDBCOMMAND_H_
#define _GDBCOMMAND_H_

#include "dbgcommand.h"

class Breakpoint;
class VarItem;

// sigh - namespace's don't work on some of the older compilers
enum GDBCmd
{
  BLOCK_START     = '\32',
  SRC_POSITION    = '\32',    // Hmmm, same value may not work for all compilers

  BPLIST          = 'B',
  SET_BREAKPT     = 'b',

  DATAREQUEST     = 'D',
  DISASSEMBLE     = 'd',

  FRAME           = 'F',
  FILE_START      = 'f',

  INITIALISE      = 'I',
  IDLE            = 'i',

  LOCALS          = 'L',
  LIBRARIES       = 'l',

  MEMDUMP         = 'M',

  RUN             = 'R',
  REGISTERS       = 'r',

  PROGRAM_STOP    = 'S',
  SHARED_CONT     = 's',

  THREAD          = 'T',
  BACKTRACE       = 't',

  SETWATCH        = 'W',
  UNSETWATCH      = 'w',

  DETACH          = 'z',

  WAIT            = '0'
};

#define RUNCMD      (true)
#define NOTRUNCMD   (false)
#define INFOCMD     (true)
#define NOTINFOCMD  (false)

/**
 * @author John Birch
 */

class GDBCommand : public DbgCommand
{
public:
    GDBCommand(const QCString& command, bool isRunCmd=false, bool isInfoCmd=true,
               char prompt=WAIT);
    virtual ~GDBCommand();
    
private:
    static QCString idlePrompt_;
};

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class GDBItemCommand : public GDBCommand
{
public:
    GDBItemCommand(VarItem *item, const QCString &command,
                   bool isRunCmd=false, char prompt=DATAREQUEST);
    virtual ~GDBItemCommand();
    
    VarItem *getItem()      { return item_; }

private:
    VarItem *item_;
};

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class GDBPointerCommand : public GDBItemCommand
{
public:
  GDBPointerCommand(VarItem *item);
  virtual ~GDBPointerCommand();
};

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
//class GDBReferenceCommand : public GDBItemCommand
//{
//public:
//  GDBReferenceCommand(VarItem* item);
//	virtual ~GDBReferenceCommand();
//};

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/
class GDBSetBreakpointCommand : public GDBCommand
{
public:
    GDBSetBreakpointCommand(const QCString& setCommand, int key);
    virtual ~GDBSetBreakpointCommand();
    
    int getKey() const        { return key_; }
    
private:
    int key_;
};

/***************************************************************************/
/***************************************************************************/
/***************************************************************************/

#endif
