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

#include <qdom.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qregexp.h>
#include <kbuttonbox.h>
#include <kdebug.h>
#include <kdialog.h>
#include <keditlistbox.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <knotifyclient.h>
#include <kservice.h>

#include "domutil.h"
#include "misc.h"
#include "addprefixdlg.h"
#include "autoprojectpart.h"
#include "autoprojectwidget.h"
#include "subprojectoptionsdlg.h"


SubprojectOptionsDialog::SubprojectOptionsDialog(AutoProjectPart *part, AutoProjectWidget *widget,
                                                 SubprojectItem *item, QWidget *parent, const char *name)
    : SubprojectOptionsDialogBase(parent, name, true)
{
    setCaption(i18n("Subproject options for '%1'").arg(item->subdir));

    subProject = item;
    m_widget = widget;
    m_part = part;

    QFontMetrics fm(cflags_edit->fontMetrics());
    int wid = fm.width('X')*35;
    cflags_edit->setMinimumWidth(wid);
    cxxflags_edit->setMinimumWidth(wid);
    fflags_edit->setMinimumWidth(wid);

    QDomDocument &dom = *part->projectDom();
    if (!KService::serviceByName(DomUtil::readEntry(dom, "/kdevautoproject/compiler/ccompiler")))
        cflags_button->setEnabled(false);
    if (!KService::serviceByName(DomUtil::readEntry(dom, "/kdevautoproject/compiler/cxxcompiler")))
        cxxflags_button->setEnabled(false);
    if (!KService::serviceByName(DomUtil::readEntry(dom, "/kdevautoproject/compiler/f77compiler")))
        fflags_button->setEnabled(false);

    insideinc_listview->header()->hide();
    outsideinc_listview->header()->hide();
    buildorder_listview->header()->hide();
    
    insideinc_listview->setSorting(-1);
    outsideinc_listview->setSorting(-1);
    prefix_listview->setSorting(-1);
    buildorder_listview->setSorting(-1);
    
    // Insert all subdirectories as possible include directories
    QStringList l = widget->allSubprojects();
    QCheckListItem *lastItem = 0;
    QStringList::ConstIterator it;
    for (it = l.begin(); it != l.end(); ++it) {
        QCheckListItem *clitem = new QCheckListItem(insideinc_listview, *it, QCheckListItem::CheckBox);
        if (lastItem)
            clitem->moveItem(lastItem);
        lastItem = clitem;
    }
        
    readConfig();
}


SubprojectOptionsDialog::~SubprojectOptionsDialog()
{}


void SubprojectOptionsDialog::readConfig()
{

    cflags_edit->setText(subProject->variables["AM_CFLAGS"]);
    cxxflags_edit->setText(subProject->variables["AM_CXXFLAGS"]);
    fflags_edit->setText(subProject->variables["AM_FFLAGS"]);

    QCString includes = subProject->variables["INCLUDES"];
    QStringList includeslist = QStringList::split(QRegExp("[ \t]"), QString(includes));

    QListViewItem *lastItem = 0;
    QStringList::Iterator it;
    for (it = includeslist.begin(); it != includeslist.end(); ++it) {
        QCheckListItem *clitem = static_cast<QCheckListItem*>(insideinc_listview->firstChild());
        while (clitem) {
            if (*it == ("-I$(top_srcdir)/" + clitem->text())) {
                clitem->setOn(true);
                break;
            }
            clitem = static_cast<QCheckListItem*>(clitem->nextSibling());
        }
        if (!clitem) {
            QListViewItem *item = new QListViewItem(outsideinc_listview, *it);
            if (lastItem)
                item->moveItem(lastItem);
            lastItem = item;
        }
    }
    
    QMap<QCString, QCString>::ConstIterator it2;
    for (it2 = subProject->prefixes.begin(); it2 != subProject->prefixes.end(); ++it2)
        new QListViewItem(prefix_listview, it2.key(), it2.data());

    QCString subdirs = subProject->variables["SUBDIRS"];
    kdDebug() << "Subdirs variable: " << subdirs << endl;
    QStringList subdirslist = QStringList::split(QRegExp("[ \t]"), QString(subdirs));
    lastItem = 0;
    for (it = subdirslist.begin(); it != subdirslist.end(); ++it) {
        QListViewItem *item = new QListViewItem(buildorder_listview, *it);
        if (lastItem)
            item->moveItem(lastItem);
        lastItem = item;
    }
}


void SubprojectOptionsDialog::storeConfig()
{
    QMap<QCString, QCString> replaceMap;
    
    QCString old_cflags = subProject->variables["AM_CFLAGS"];
    QCString new_cflags = cflags_edit->text().latin1();
    if (new_cflags != old_cflags) {
        subProject->variables["AM_CFLAGS"] = new_cflags;
        replaceMap.insert(QCString("AM_CFLAGS"), new_cflags);
    }

    QCString old_cxxflags = subProject->variables["AM_CXXFLAGS"];
    QCString new_cxxflags = cxxflags_edit->text().latin1();
    if (new_cxxflags != old_cxxflags) {
        subProject->variables["AM_CXXFLAGS"] = new_cxxflags;
        replaceMap.insert(QCString("AM_CXXFLAGS"), new_cxxflags);
    }

    QCString old_fflags = subProject->variables["AM_FFLAGS"];
    QCString new_fflags = fflags_edit->text().latin1();
    if (new_fflags != old_fflags) {
        subProject->variables["AM_FFLAGS"] = new_fflags;
        replaceMap.insert(QCString("AM_FFLAGS"), new_fflags);
    }

    QStringList includeslist;
    QCheckListItem *clitem = static_cast<QCheckListItem*>(insideinc_listview->firstChild());
    while (clitem) {
        if (clitem->isOn())
            includeslist.append("-I$(top_srcdir)/" + clitem->text());
        clitem = static_cast<QCheckListItem*>(clitem->nextSibling());
    }
    clitem = static_cast<QCheckListItem*>(outsideinc_listview->firstChild());
    while (clitem) {
        includeslist.append(clitem->text());
        clitem = static_cast<QCheckListItem*>(clitem->nextSibling());
    }
    QCString includes = includeslist.join(" ").latin1();
    subProject->variables["INCLUDES"] = includes;
    replaceMap.insert("INCLUDES", includes);

    subProject->prefixes.clear();
    for (QListViewItem *item = prefix_listview->firstChild();
         item; item = item->nextSibling()) {
        QCString key = item->text(0).latin1();
        QCString data = item->text(1).latin1();
        subProject->prefixes[key] = data;
        replaceMap.insert(key + "dir", data);
    }
    // FIXME: take removed prefixes into account

    QStringList subdirslist;
    for (QListViewItem *item = buildorder_listview->firstChild();
         item; item = item->nextSibling())
        subdirslist.append(item->text(0));
    QCString subdirs = subdirslist.join(" ").latin1();
    kdDebug() << "New subdirs variable: " << subdirs << endl;
    subProject->variables["SUBDIRS"] = subdirs;
    replaceMap.insert("SUBDIRS", subdirs);
    
    AutoProjectTool::modifyMakefileam(subProject->path + "/Makefile.am", replaceMap);
}


void SubprojectOptionsDialog::cflagsClicked()
{
    QString ccompiler = DomUtil::readEntry(*m_part->projectDom(), "/kdevautoproject/compiler/ccompiler");
    QString new_cflags = AutoProjectTool::execFlagsDialog(ccompiler, cflags_edit->text(), this);
    if (!new_cflags.isNull())
        cflags_edit->setText(new_cflags);
}


void SubprojectOptionsDialog::cxxflagsClicked()
{
    QString cxxcompiler = DomUtil::readEntry(*m_part->projectDom(), "/kdevautoproject/compiler/cxxcompiler");
    QString new_cxxflags = AutoProjectTool::execFlagsDialog(cxxcompiler, cxxflags_edit->text(), this);
    if (!new_cxxflags.isNull())
        cxxflags_edit->setText(new_cxxflags);
}


void SubprojectOptionsDialog::fflagsClicked()
{
    QString f77compiler = DomUtil::readEntry(*m_part->projectDom(), "/kdevautoproject/compiler/f77compiler");
    QString new_fflags = AutoProjectTool::execFlagsDialog(f77compiler, fflags_edit->text(), this);
    if (!new_fflags.isNull())
        fflags_edit->setText(new_fflags);
}


void SubprojectOptionsDialog::insideMoveUpClicked()
{
    if (insideinc_listview->currentItem() == insideinc_listview->firstChild()) {
        KNotifyClient::beep();
        return;
    }

    QListViewItem *item = insideinc_listview->firstChild();
    while (item->nextSibling() != insideinc_listview->currentItem())
        item = item->nextSibling();
    item->moveItem(insideinc_listview->currentItem());
}


void SubprojectOptionsDialog::insideMoveDownClicked()
{
   if (insideinc_listview->currentItem()->nextSibling() == 0) {
        KNotifyClient::beep();
        return;
   }

   insideinc_listview->currentItem()->moveItem(insideinc_listview->currentItem()->nextSibling());
}


void SubprojectOptionsDialog::outsideMoveUpClicked()
{
    if (outsideinc_listview->currentItem() == outsideinc_listview->firstChild()) {
        KNotifyClient::beep();
        return;
    }

    QListViewItem *item = outsideinc_listview->firstChild();
    while (item->nextSibling() != outsideinc_listview->currentItem())
        item = item->nextSibling();
    item->moveItem(outsideinc_listview->currentItem());
}


void SubprojectOptionsDialog::outsideMoveDownClicked()
{
   if (outsideinc_listview->currentItem()->nextSibling() == 0) {
        KNotifyClient::beep();
        return;
   }

   outsideinc_listview->currentItem()->moveItem(outsideinc_listview->currentItem()->nextSibling());
}


void SubprojectOptionsDialog::outsideAddClicked()
{
    bool ok;
    QString dir = KLineEditDlg::getText(i18n("Add include directory:"), "-I", &ok, 0);
    if (ok && !dir.isEmpty() && dir != "-I")
        new QListViewItem(outsideinc_listview, dir);
}


void SubprojectOptionsDialog::outsideRemoveClicked()
{
    delete outsideinc_listview->currentItem();
}


void SubprojectOptionsDialog::addPrefixClicked()
{
    AddPrefixDialog dlg;
    if (!dlg.exec())
        return;
    
    new QListViewItem(prefix_listview, dlg.name(), dlg.path());
}


void SubprojectOptionsDialog::removePrefixClicked()
{
    delete prefix_listview->currentItem();
}


void SubprojectOptionsDialog::buildorderMoveUpClicked()
{
    if (buildorder_listview->currentItem() == buildorder_listview->firstChild()) {
        KNotifyClient::beep();
        return;
    }

    QListViewItem *item = buildorder_listview->firstChild();
    while (item->nextSibling() != buildorder_listview->currentItem())
        item = item->nextSibling();
    item->moveItem(buildorder_listview->currentItem());
}


void SubprojectOptionsDialog::buildorderMoveDownClicked()
{
   if (buildorder_listview->currentItem()->nextSibling() == 0) {
        KNotifyClient::beep();
        return;
   }

   buildorder_listview->currentItem()->moveItem(buildorder_listview->currentItem()->nextSibling());
}


void SubprojectOptionsDialog::accept()
{
    storeConfig();
    QDialog::accept();
}

#include "subprojectoptionsdlg.moc"
