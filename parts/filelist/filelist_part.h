/***************************************************************************
 *   Copyright (C) 2004 by Jens Dagerbo                                    *
 *   jens.dagerbo@swipnet.se                                               *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef __KDEVPART_FILELIST_H__
#define __KDEVPART_FILELIST_H__


#include <qguardedptr.h>
#include <kdevplugin.h>


class FileListWidget;
class KURL::List;
/*
// @fixme - this should be in kdevpartcontroller
enum DocumentState
{
	Clean,
	Modified,
	Dirty,
	DirtyAndModified
};
*/
class FileListPart : public KDevPlugin
{
  Q_OBJECT

public:
   
	FileListPart(QObject *parent, const char *name, const QStringList &);
	~FileListPart();
	KURL::List openFiles();

private:
	QGuardedPtr<FileListWidget> m_widget;

};


#endif

// kate: space-indent off; indent-width 4; tab-width 4; show-tabs off;
