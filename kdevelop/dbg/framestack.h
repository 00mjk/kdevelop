/***************************************************************************
                          framestack.h  -  description                              
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

#ifndef FRAMESTACK_H
#define FRAMESTACK_H

#include <qlistview.h>

/**
  *@author John Birch
  */

class FrameStack;

// **************************************************************************
// **************************************************************************
// **************************************************************************

class ThreadStackItem : public QListViewItem
{
public:

  ThreadStackItem(FrameStack* parent, const QString& threadDesc);
  virtual ~ThreadStackItem();

  void setOpen(bool open);
  QListViewItem* lastChild() const;

  int threadNo()    { return threadNo_; }

private:
  int threadNo_;
};

// **************************************************************************
// **************************************************************************
// **************************************************************************

class FrameStackItem : public QListViewItem
{
public:

  FrameStackItem(FrameStack* parent, const QString& frameDesc);
  FrameStackItem(ThreadStackItem* parent, const QString& frameDesc);
  virtual ~FrameStackItem();

  void setOpen(bool open);
  QListViewItem* lastChild() const;

  int frameNo()         { return frameNo_; }
  int threadNo()        { return threadNo_; }

private:
  int frameNo_;
  int threadNo_;
};

// **************************************************************************
// **************************************************************************
// **************************************************************************

class FrameStack : public QListView
{
  Q_OBJECT

public:
  FrameStack( QWidget * parent=0, const char * name=0, WFlags f=0 );
  virtual ~FrameStack();
  void parseGDBThreadList(char* str);
  void parseGDBBacktraceList(char* str);

  QListViewItem* lastChild() const;
  void clear();

  ThreadStackItem* findThread(int threadNo);
  FrameStackItem* findFrame(int frameNo, int threadNo);

  QCString getFrameParams(int frameNo, int threadNo);
  QString getFrameName(int frameNo, int threadNo);

  int viewedThread()         { return viewedThread_ ? viewedThread_->threadNo() : -1; }

public slots:
  void slotSelectFrame(int frameNo, int threadNo);
  void slotSelectionChanged(QListViewItem *thisItem);

signals:
  void selectFrame(int frameNo, int threadNo, bool needFrames);

private:
  ThreadStackItem*  viewedThread_;
  ThreadStackItem*  stoppedAtThread_;
  int               currentFrame_;
};

#endif
