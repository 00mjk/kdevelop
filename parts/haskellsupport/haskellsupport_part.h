/***************************************************************************
                          haskellsupport_part.h  -  description
                             -------------------
    begin                : Mon Aug 11 2003
    copyright            : (C) 2003 Peter Robinson
    email                : listener@thaldyron.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KDEVPART_HASKELLSUPPORT_PART_H
#define KDEVPART_HASKELLSUPPORT_PART_H

#include <qwidget.h>
#include <qguardedptr.h>

#include "kdevlanguagesupport.h"

class HaskellSupportWidget; // @todo remove this
class KDialogBase;
class QPopupMenu;
class Context;

class HaskellSupportPart : public KDevLanguageSupport
{
   Q_OBJECT

public:
	HaskellSupportPart(QObject *parent, const char *name, const QStringList &);
	~HaskellSupportPart();

  	virtual Features features();
  	virtual KMimeType::List mimeTypes();

private slots:
	void projectOpened();
	void projectClosed();
  	void savedFile(const QString &fileName);
  	void configWidget(KDialogBase *dlg);
	void projectConfigWidget(KDialogBase *dlg);
  	void contextMenu(QPopupMenu *popup, const Context *context);

  	void addedFilesToProject(const QStringList &fileList);
  	void removedFilesFromProject(const QStringList &fileList);
  	void slotProjectCompiled();

	void slotInitialParse();

private:
  	void maybeParse(const QString &fileName);
  	void parse(const QString &fileName);

  	QGuardedPtr<HaskellSupportWidget> m_widget;
  	bool m_projectClosed;
  	QStringList m_projectFileList;
};

#endif
