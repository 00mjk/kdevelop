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

#ifndef _FRAMESTACKWIDGET_H_
#define _FRAMESTACKWIDGET_H_

#include <qlistbox.h>
#include <qstrlist.h>

/**
 * @author John Birch
 */
class FramestackWidget : public QListBox
{
    Q_OBJECT
    
public:
    FramestackWidget( QWidget *parent=0, const char *name=0 );
    virtual ~FramestackWidget();

    void parseGDBBacktraceList(char *str);
    QCString getFrameParams(int frame);
    QString getFrameName(int frame);
    
public slots:
    void slotHighlighted(int index);
    void slotSelectFrame(int frame);

signals:
    void selectFrame(int selectFrame);

private:
    int currentFrame_;
    QStrList *currentList_;
};

#endif
