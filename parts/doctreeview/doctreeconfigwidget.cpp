/***************************************************************************
 *   Copyright (C) 1999-2001 by Bernd Gehrmann                             *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#define GIDEON

#include <qcheckbox.h>
#include <qlabel.h>
#include <qlistview.h>
#include <qpushbutton.h>
#include <qheader.h>
#include <qlayout.h>
#include <qvbox.h>
#include <kdebug.h>
#include <kdialog.h>
#include <kinstance.h>
#include <klocale.h>
#include <kglobal.h>
#include <kstddirs.h>
#include <ksimpleconfig.h>
#include <kprocess.h>

#include "adddocitemdlg.h"
#include "misc.h"
#include "doctreeviewfactory.h"
#include "doctreeviewwidget.h"
#include "doctreeconfigwidget.h"


class DocTreeConfigListItem : public QCheckListItem
{
public:
    DocTreeConfigListItem(QListView *parent, const QString &text,
                          const QString &id, int number)
        : QCheckListItem(parent, text, CheckBox)
        { setOn(true); idnt = id; no = number; }
    QString ident()
        { return idnt; }
    virtual QString key(int, bool) const
        { return QString().setNum(no); }
private:
    QString idnt;
    int no;
};



DocTreeConfigWidget::DocTreeConfigWidget(DocTreeViewWidget *widget,
                                         QWidget *parent, const char *name)
    : QTabWidget(parent, name)
{
    m_widget = widget;

#ifndef GIDEON
    addTab(kdevelopTab=createKDevelopTab(), i18n("KDevelop"));
#endif
    addTab(librariesTab=createLibrariesTab(), i18n("&Qt/KDE libraries"));
    addTab(bookmarksTab=createBookmarksTab(), i18n("&Bookmarks"));
    addTab(indexTab=createIndexTab(), i18n("Search &index"));
    
    readConfig();
}


DocTreeConfigWidget::~DocTreeConfigWidget()
{}


QWidget *DocTreeConfigWidget::createKDevelopTab()
{
#ifndef GIDEON
    QHBox *hbox = new QHBox(this);
    hbox->setMargin(2*KDialog::marginHint());
    hbox->setSpacing(KDialog::spacingHint());
    (void) new QLabel(i18n("Items shown in the \n"
                           "KDevelop section \n"
                           "of the documentation tree:"), hbox);
    kdevelop_view = new QListView(hbox);
    kdevelop_view->addColumn("");
    kdevelop_view->header()->hide();

    return hbox;
#else
    return 0;
#endif
}


QWidget *DocTreeConfigWidget::createLibrariesTab()
{
    QWidget *w = new QWidget(this, "libraries tab");
    
    QLabel *libraries_label = new QLabel(i18n("Items shown in the Qt/KDE Libraries section "
                                              "of the documentation tree:"), w);
    
    libraries_view = new QListView(w);
    libraries_view->addColumn("");
    libraries_view->header()->hide();

    QBoxLayout *layout = new QVBoxLayout(w, 2*KDialog::marginHint(), KDialog::spacingHint());
    layout->addWidget(libraries_label);
    layout->addWidget(libraries_view);
    
    return w;
}


QWidget *DocTreeConfigWidget::createBookmarksTab()
{
    QWidget *w = new QWidget(this, "bookmarks tab");
    
    QLabel *bookmarks_label = new QLabel(i18n("Items shown in the Bookmarks section of the documentation tree:"), w);
    
    bookmarks_view = new QListView(w);
    QFontMetrics fm(bookmarks_view->fontMetrics());
    bookmarks_view->setMinimumWidth(fm.width('X')*35);
    bookmarks_view->setAllColumnsShowFocus(true);
    bookmarks_view->setColumnWidth(0, 70);
    bookmarks_view->setColumnWidth(1, 170);
    bookmarks_view->addColumn(i18n("Title"));
    bookmarks_view->addColumn(i18n("URL"));
    
    QVBox *buttonbox = new QVBox(w);
    buttonbox->setMargin(KDialog::spacingHint());
    connect( new QPushButton(i18n("&Add..."), buttonbox), SIGNAL(clicked()),
             this, SLOT(addBookmarkClicked()) );
    connect( new QPushButton(i18n("&Remove"), buttonbox), SIGNAL(clicked()),
             this, SLOT(removeBookmarkClicked()) );

    QGridLayout *layout = new QGridLayout(w, 2, 2, 2*KDialog::marginHint(), KDialog::spacingHint());
    layout->addMultiCellWidget(bookmarks_label, 0, 0, 0, 1);
    layout->addWidget(bookmarks_view, 1, 0);
    layout->addWidget(buttonbox, 1, 1);
    
    return w;
}


QWidget *DocTreeConfigWidget::createIndexTab()
{
    QWidget *w = new QWidget(this, "index tab");

    QLabel *info_label = new QLabel(i18n("Here you can configure which files are included "
                                         "in the index created for full text searching."), w);
    
    indexshownlibs_box = new QCheckBox(i18n("Index &libraries shown in the documentation tree"), w);
    indexhiddenlibs_box = new QCheckBox(i18n("Index &other libraries"), w);
    indexbookmarks_box = new QCheckBox(i18n("Index files in the &Bookmarks section of the documentation tree"), w);

    QPushButton *update_button = new QPushButton(i18n("&Update index now"), w);
    connect( update_button, SIGNAL(clicked()), this, SLOT(updateIndexClicked()) );
    
    QBoxLayout *layout = new QVBoxLayout(w, 2*KDialog::marginHint(), KDialog::spacingHint());
    layout->addStretch(1);
    layout->addWidget(info_label);
    layout->addSpacing(5);
    layout->addWidget(indexshownlibs_box);
    layout->addWidget(indexhiddenlibs_box);
    layout->addWidget(indexbookmarks_box);
    layout->addStretch(2);
    layout->addWidget(update_button, 0, AlignCenter);
    layout->addStretch(2);
    
    return w;
}


void DocTreeConfigWidget::showPage(DocTreeConfigWidget::Page page)
{
    QTabWidget::showPage((page==KDevelop)? kdevelopTab :
                         (page==Libraries)? librariesTab : bookmarksTab);
}


void DocTreeConfigWidget::readConfig()
{
#ifndef GIDEON
    QString path = locate("appdata", "tools/documentation");
    KSimpleConfig docconfig(path);
    // Read in possible items for the KDevelop tree
    docconfig.setGroup("Contents");
    QStringList kdevelopEntries = docconfig.readListEntry("KDevelopEntries");
    int kdevelopPos = 0;
    for (QStringList::Iterator it = kdevelopEntries.begin();
         it != kdevelopEntries.end();
         ++it)
	{
            docconfig.setGroup("KDevelop-" + (*it));
            QString name = docconfig.readEntry("Name"); 
            (void) new DocTreeConfigListItem(kdevelop_view, name, (*it), kdevelopPos);
	    kdDebug(9002) << "Inserting " << name << endl;
            ++kdevelopPos;
	}
#endif
    // Read in possible items for the Libraries tree
    QStringList librariesTitle, librariesURL, librariesHidden;
    DocTreeViewTool::getAllLibraries(&librariesTitle, &librariesURL);
    DocTreeViewTool::getHiddenLibraries(&librariesHidden);

    int librariesPos = 0;
    QStringList::Iterator lit1, lit2;
    for (lit1 = librariesTitle.begin(), lit2 = librariesURL.begin();
         lit1 != librariesTitle.end() && lit2 != librariesURL.end();
         ++lit1, ++lit2) {
        (void) new DocTreeConfigListItem(libraries_view, (*lit1), (*lit2), librariesPos);
        kdDebug(9002) << "Insert " << (*lit2) << endl;
        ++librariesPos;
    }

#ifndef GIDEON
    // Enable/disable items in the KDevelop tree
    DocTreeViewTool::getHiddenKDevelop(&kdevelopHidden);
    for (QStringList::Iterator it = kdevelopHidden.begin(); it != kdevelopHidden.end();
         ++it)
	{
            QListViewItem *item = kdevelop_view->firstChild();
            for (; item; item = item->nextSibling())
                {
                    DocTreeConfigListItem *citem = static_cast<DocTreeConfigListItem*>(item);
                    kdDebug(9002) << "Checking " << citem->ident() << " with " << (*it) << endl;
                    if (citem->ident() == (*it))
                        citem->setOn(false);
                }
        }
#endif
    
    // Enable/disable items in the Libraries tree
    DocTreeViewTool::getHiddenLibraries(&librariesHidden);
    for (QStringList::Iterator it = librariesHidden.begin(); it != librariesHidden.end();
         ++it)
	{
            QListViewItem *item = libraries_view->firstChild();
            for (; item; item = item->nextSibling())
                {
                    DocTreeConfigListItem *citem = static_cast<DocTreeConfigListItem*>(item);
                    // kdDebug(9002) << "Checking " << citem->ident() << " with " << (*it) << endl;
                    if (citem->ident() == (*it))
                        citem->setOn(false);
                }
        }

    // Read in configuration of the Bookmarks tree
    // Here we store what we _want_ to see, in contrast to the KDevelop/Libraries
    // sections where we store what  we _don't_ want to see
    QStringList bookmarksTitle, bookmarksURL;
    DocTreeViewTool::getBookmarks(&bookmarksTitle, &bookmarksURL);
    QStringList::Iterator oit1, oit2;
    for (oit1 = bookmarksTitle.begin(), oit2 = bookmarksURL.begin();
         oit1 != bookmarksTitle.end() && oit2 != bookmarksURL.end();
         ++oit1, ++oit2) {
        new QListViewItem(bookmarks_view, *oit1, *oit2);
    }

    // Read configuration of the index tab
    bool indexShownLibs, indexHiddenLibs, indexBookmarks;
    DocTreeViewTool::getIndexOptions(&indexShownLibs, &indexHiddenLibs, &indexBookmarks);
    indexshownlibs_box->setChecked(indexShownLibs);
    indexhiddenlibs_box->setChecked(indexHiddenLibs);
    indexbookmarks_box->setChecked(indexBookmarks);
}


void DocTreeConfigWidget::storeConfig()
{
    kdDebug() << "DocTreeConfigWidget::storeConfig()" << endl;

#ifndef GIDEON
    // Save KDevelop section
    QStringList kdevelopHidden;
    {
        QListViewItem *item = kdevelop_view->firstChild();
        for (; item; item = item->nextSibling())
            {
                DocTreeConfigListItem *citem = static_cast<DocTreeConfigListItem*>(item);
                if (!citem->isOn())
                    kdevelopHidden.append(citem->ident());
            }
    }
    DocTreeViewTool::setHiddenKDevelop(kdevelopHidden);
#endif
    
    // Save Libraries section
    QStringList librariesHidden;
    {
        QListViewItem *item = libraries_view->firstChild();
        for (; item; item = item->nextSibling())
            {
                DocTreeConfigListItem *citem = static_cast<DocTreeConfigListItem*>(item);
                if (!citem->isOn())
                    librariesHidden.append(citem->ident());
            }
    }
    DocTreeViewTool::setHiddenLibraries(librariesHidden);
    
    // Save Bookmarks section
    QStringList bookmarksTitle, bookmarksURL;
    {
        QListViewItem *item = bookmarks_view->firstChild();
        for (; item; item = item->nextSibling())
            {
                bookmarksTitle.append(item->text(0));
                bookmarksURL.append(item->text(1));
            }
    }
    DocTreeViewTool::setBookmarks(bookmarksTitle, bookmarksURL);

    // Save Index section
    DocTreeViewTool::setIndexOptions(indexshownlibs_box->isChecked(),
                                     indexhiddenlibs_box->isChecked(),
                                     indexbookmarks_box->isChecked());
}


void DocTreeConfigWidget::addBookmarkClicked()
{
    AddDocItemDialog dlg;
    if (!dlg.exec())
        return;

    (void) new QListViewItem(bookmarks_view, dlg.title(), dlg.url());
}


void DocTreeConfigWidget::removeBookmarkClicked()
{
    delete bookmarks_view->currentItem();
}


void DocTreeConfigWidget::updateIndexClicked()
{
    // I'm not sure if storing the configuration here is compliant
    // with user interface guides, but I see no easy way around
    storeConfig();
    
    DocTreeViewFactory::instance()->config()->sync();
    KProcess proc;
    proc << "gideon-index";
    proc.start(KProcess::DontCare);
}


void DocTreeConfigWidget::accept()
{
    storeConfig();
    m_widget->configurationChanged();
}

#include "doctreeconfigwidget.moc"
