/* This file is part of the KDE project
   Copyright (C) 2002 John Firebaugh <jfirebaugh@kde.org>

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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef _PROCESSLINEMAKER_H_
#define _PROCESSLINEMAKER_H_

#include <QObject>
#include "kdevexport.h"

/**
@file processlinemaker.h
Utility objects for process output views.
*/

class K3Process;
class QString;

/**
Convenience class to catch output of K3Process.
*/
class KDEVUTIL_EXPORT ProcessLineMaker : public QObject
{
Q_OBJECT

public:
	ProcessLineMaker();
	ProcessLineMaker( const K3Process* );
	
	void clearBuffers();

public slots:
	void slotReceivedStdout(const QString&);
	void slotReceivedStderr(const QString&);

	void slotReceivedStdout(const char*);
	void slotReceivedStderr(const char*);

protected slots:
	void slotReceivedStdout(K3Process *, char *buffer, int buflen);
	void slotReceivedStderr(K3Process *, char *buffer, int buflen);
        
signals:
	void receivedStdoutLine( const QString& line );
	void receivedStderrLine( const QString& line );
        
private:
	QString stdoutbuf;
	QString stderrbuf;
};

#endif
