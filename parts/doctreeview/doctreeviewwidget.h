/***************************************************************************
 *   Copyright (C) 1999-2001 by Bernd Gehrmann                            *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _DOCTREEWIDGET_H_
#define _DOCTREEWIDGET_H_

#include "klistview.h"


class DocTreeKDELibsFolder;
class DocTreeBookmarksFolder;
class DocTreeProjectFolder;
class DocTreeDocbaseFolder;
class DocTreeTocFolder;
class DocTreeViewPart;
class KDevProject;
class CustomizeDialog;


class DocTreeViewWidget : public KListView
{
    Q_OBJECT
    
public: 
    DocTreeViewWidget(DocTreeViewPart *part);
    ~DocTreeViewWidget();

    void configurationChanged();
    void projectChanged(KDevProject *project);
    
    static QString locatehtml(const QString &fileName);

private slots:
    void slotConfigure();
    void slotItemExecuted(QListViewItem *item);
    void slotContextMenu(KListView *, QListViewItem *item, const QPoint &p);
    void refresh();
	
private: 
    QListViewItem *contextItem;
    DocTreeKDELibsFolder *folder_kdelibs;
    DocTreeBookmarksFolder *folder_bookmarks;
    DocTreeDocbaseFolder *folder_docbase;
    DocTreeProjectFolder *folder_project;
    QList<DocTreeTocFolder> folder_toc;
    DocTreeViewPart *m_part;
};
#endif
