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

#include "targetoptionsdlg.h"

#include <qcheckbox.h>
#include <qheader.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qlineedit.h>
#include <qregexp.h>
#include <kbuttonbox.h>
#include <kdialog.h>
#include <klineeditdlg.h>
#include <klocale.h>
#include <knotifyclient.h>

#include "autolistviewitems.h"

#include "misc.h"
#include "autoprojectwidget.h"


TargetOptionsDialog::TargetOptionsDialog(AutoProjectWidget *widget, TargetItem *item,
                                         QWidget *parent, const char *name)
    : TargetOptionsDialogBase(parent, name, true)
{
    setCaption( i18n("Target Options for '%1'").arg(item->name) );
    setIcon( SmallIcon("configure") );

    target = item;
    m_widget = widget;

    if (item->primary == "PROGRAMS") {
        insidelib_label->setText(i18n("Link convenience libraries inside project (LDADD)"));
        outsidelib_label->setText(i18n("Link libraries outside project (LDADD)"));
    }

    insidelib_listview->header()->hide();
    outsidelib_listview->header()->hide();
    insidelib_listview->setSorting(-1);
    outsidelib_listview->setSorting(-1);

    // Insert all convenience libraries as possible linked libraries
    QStringList l = widget->allLibraries();
    QStringList::ConstIterator it;
    for (it = l.begin(); it != l.end(); ++it)
        (void) new QCheckListItem(insidelib_listview, *it, QCheckListItem::CheckBox);

    readConfig();
}


TargetOptionsDialog::~TargetOptionsDialog()
{}


void TargetOptionsDialog::readConfig()
{
    QString flagsstr = target->ldflags;
    flagsstr.replace(QRegExp("$(KDE_PLUGIN)"), "-avoid-version -module -no-undefined $(KDE_RPATH)");
    QStringList l1 = QStringList::split(QRegExp("[ \t]"), flagsstr);
    QStringList::Iterator l1it;

    l1it = l1.find("-all-static");
    if (l1it != l1.end()) {
        allstatic_box->setChecked(true);
        l1.remove(l1it);
    }
    l1it = l1.find("-avoid-version");
    if (l1it != l1.end()) {
        avoidversion_box->setChecked(true);
        l1.remove(l1it);
    }
    l1it = l1.find("-module");
    if (l1it != l1.end()) {
        module_box->setChecked(true);
        l1.remove(l1it);
    }
    l1it = l1.find("-no-undefined");
    if (l1it != l1.end()) {
        noundefined_box->setChecked(true);
        l1.remove(l1it);
    }
    ldflagsother_edit->setText(l1.join(" "));
    dependencies_edit->setText(target->dependencies);

    QString addstr = (target->primary == "PROGRAMS")? target->ldadd : target->libadd;
    QStringList l2 = QStringList::split(QRegExp("[ \t]"), addstr);

    bool inlistItem;
    QListViewItem *lastItem = 0;
    QStringList::Iterator l2it;
    QCheckListItem *flitem = static_cast<QCheckListItem*>(insidelib_listview->firstChild());
    for (l2it = l2.begin(); l2it != l2.end(); ++l2it) {
        inlistItem = false;
        QCheckListItem *clitem = static_cast<QCheckListItem*>(insidelib_listview->firstChild());
        if (flitem) {
            while (clitem) {
                if (*l2it == ("$(top_builddir)/" + clitem->text())) {
                    clitem->setOn(true);
                    // move this item to the "top of the list"
                    if (flitem != clitem)
                      clitem->moveItem(flitem);
                    // move the "top of the list" one item down
                    flitem = static_cast<QCheckListItem*>(flitem->nextSibling());
                    inlistItem = true;
                    break;
                }
                clitem = static_cast<QCheckListItem*>(clitem->nextSibling());
            }
        }
        if ( inlistItem == false ) {
            QListViewItem *item = new QListViewItem(outsidelib_listview, *l2it);
            if (lastItem)
                item->moveItem(lastItem);
            lastItem = item;
        }
    }
}


void TargetOptionsDialog::storeConfig()
{
    QStringList flagslist;
    if (allstatic_box->isChecked())
        flagslist.append("-all-static");
    if (avoidversion_box->isChecked())
        flagslist.append("-avoid-version");
    if (module_box->isChecked())
        flagslist.append("-module");
    if (noundefined_box->isChecked())
        flagslist.append("-no-undefined");
    flagslist.append(ldflagsother_edit->text());
    QString new_ldflags = flagslist.join(" ");

    QStringList liblist;
    QCheckListItem *clitem = static_cast<QCheckListItem*>(insidelib_listview->firstChild());
    while (clitem) {
        if( clitem->isOn() )
            liblist.append("$(top_builddir)/" + clitem->text());
        clitem = static_cast<QCheckListItem*>(clitem->nextSibling());
    }
    clitem = static_cast<QCheckListItem*>(outsidelib_listview->firstChild());
    while (clitem) {
        liblist.append(clitem->text());
        clitem = static_cast<QCheckListItem*>(clitem->nextSibling());
    }
    QString new_addstr = liblist.join(" ");

    QString canonname = AutoProjectTool::canonicalize(target->name);
    QMap<QString, QString> replaceMap;

    if (target->primary == "PROGRAMS") {
        QString old_ldadd = target->ldadd;
        if (new_addstr != old_ldadd) {
            target->ldadd = new_addstr;
            replaceMap.insert(canonname + "_LDADD", new_addstr);
        }
    }

    if (target->primary == "LIBRARIES" || target->primary == "LTLIBRARIES") {
        QString old_libadd = target->libadd;
        if (new_addstr != old_libadd) {
            target->libadd = new_addstr;
            replaceMap.insert(canonname + "_LIBADD", new_addstr);
        }
    }

    QString old_ldflags = target->ldflags;
    if (new_ldflags != old_ldflags) {
        target->ldflags = new_ldflags;
        replaceMap.insert(canonname + "_LDFLAGS", new_ldflags);
    }

    QString new_dependencies = dependencies_edit->text();
    QString old_dependencies = target->dependencies;
    if (new_dependencies != old_dependencies) {
        target->dependencies = new_dependencies;
        if (!new_dependencies.isEmpty())
            replaceMap.insert(canonname + "_DEPENDENCIES", new_dependencies);
    }

    // We can safely assume that this target is in the active sub project
    AutoProjectTool::modifyMakefileam(m_widget->subprojectDirectory() + "/Makefile.am", replaceMap);
}


void TargetOptionsDialog::insideMoveUpClicked()
{
    if (!insidelib_listview->currentItem())
        return;
    if (insidelib_listview->currentItem() == insidelib_listview->firstChild()) {
        KNotifyClient::beep();
        return;
    }

    QListViewItem *item = insidelib_listview->firstChild();
    while (item->nextSibling() != insidelib_listview->currentItem())
        item = item->nextSibling();
    item->moveItem(insidelib_listview->currentItem());
}


void TargetOptionsDialog::insideMoveDownClicked()
{
   if (!insidelib_listview->currentItem())
       return;

   if (insidelib_listview->currentItem()->nextSibling() == 0) {
        KNotifyClient::beep();
        return;
   }

   insidelib_listview->currentItem()->moveItem(insidelib_listview->currentItem()->nextSibling());
}


void TargetOptionsDialog::outsideMoveUpClicked()
{
    if (!outsidelib_listview->currentItem())
        return;
    if (outsidelib_listview->currentItem() == outsidelib_listview->firstChild()) {
        KNotifyClient::beep();
        return;
    }

    QListViewItem *item = outsidelib_listview->firstChild();
    while (item->nextSibling() != outsidelib_listview->currentItem())
        item = item->nextSibling();
    item->moveItem(outsidelib_listview->currentItem());
}


void TargetOptionsDialog::outsideMoveDownClicked()
{
   if (!outsidelib_listview->currentItem())
       return;
   if (outsidelib_listview->currentItem()->nextSibling() == 0) {
        KNotifyClient::beep();
        return;
   }

   outsidelib_listview->currentItem()->moveItem(outsidelib_listview->currentItem()->nextSibling());
}


void TargetOptionsDialog::outsideAddClicked()
{
    bool ok;
    QString dir = KLineEditDlg::getText(i18n("Add library:"), "-l", &ok, 0);
    if (ok && !dir.isEmpty() && dir != "-l")
        new QListViewItem(outsidelib_listview, dir);
}


void TargetOptionsDialog::outsideRemoveClicked()
{
    delete outsidelib_listview->currentItem();
}


void TargetOptionsDialog::accept()
{
    storeConfig();
    QDialog::accept();
}

#include "targetoptionsdlg.moc"
