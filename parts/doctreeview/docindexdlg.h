/***************************************************************************
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _DOCINDEXDLG_H_
#define _DOCINDEXDLG_H_

#include <qdialog.h>
#include <qdom.h>
#include <qlist.h>

class QCheckBox;
class KComboBox;
class DocTreeViewPart;


class DocIndex
{
public:
    QString indexName;
    QString title;
    QString base;
    QStringList conceptNames;
    QStringList conceptUrls;
    QStringList identNames;
    QStringList identUrls;
    QStringList fileNames;
    QStringList fileUrls;
};


class DocIndexDialog : public QDialog
{
    Q_OBJECT
    
public:
    DocIndexDialog( DocTreeViewPart *part, QWidget *parent=0, const char *name=0 );
    ~DocIndexDialog();

    void projectChanged();
    
protected:
    virtual void accept();

private slots:
    void choiceChanged();
    
private:
    void readConfig();
    void storeConfig();
    void readIndexFromFile(const QString &fileName);
    void readEntryList(const QDomElement &el, QStringList *nameList, QStringList *urlList);

    KComboBox *term_combo;
    QList<QCheckBox> books_boxes;
    QCheckBox *concept_box;
    QCheckBox *ident_box;
    QCheckBox *file_box;
    QList<DocIndex> indices;
    DocTreeViewPart *m_part;
};

#endif

