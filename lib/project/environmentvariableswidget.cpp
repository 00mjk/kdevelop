/***************************************************************************
 *   Copyright (C) 2001-2002 by Bernd Gehrmann                             *
 *   bernd@kdevelop.org                                                    *
 *   Copyright (C) 2003 John Firebaugh                                     *
 *   jfirebaugh@kde.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "environmentvariableswidget.h"

#include <qcheckbox.h>
#include <qlineedit.h>
#include <qspinbox.h>
#include <qlistview.h>
#include "domutil.h"
#include "addenvvardlg.h"


void EnvironmentVariablesWidget::addVarClicked()
{
    AddEnvvarDialog dlg;
    if (QListViewItem *Item = listview->selectedItem())
    {
        dlg.setvarname(Item->text(0));
        dlg.setvalue(Item->text(1));
    }
    if (!dlg.exec())
        return;

    (void) new QListViewItem(listview, dlg.varname(), dlg.value());
}


void EnvironmentVariablesWidget::removeVarClicked()
{
    delete listview->selectedItem();
}


EnvironmentVariablesWidget::EnvironmentVariablesWidget(QDomDocument &dom, const QString &configGroup,
                                   QWidget *parent, const char *name)
    : EnvironmentVariablesWidgetBase(parent, name),
      m_dom(dom), m_configGroup(configGroup)
{
    DomUtil::PairList list =
        DomUtil::readPairListEntry(dom, configGroup, "envvar", "name", "value");
    
    QListViewItem *lastItem = 0;

    DomUtil::PairList::ConstIterator it;
    for (it = list.begin(); it != list.end(); ++it) {
        QListViewItem *newItem = new QListViewItem(listview, (*it).first, (*it).second);
        if (lastItem)
            newItem->moveItem(lastItem);
        lastItem = newItem;
    }
}


EnvironmentVariablesWidget::~EnvironmentVariablesWidget()
{}


void EnvironmentVariablesWidget::accept()
{
    DomUtil::PairList list;
    QListViewItem *item = listview->firstChild();
    while (item) {
        list << DomUtil::Pair(item->text(0), item->text(1));
        item = item->nextSibling();
    }

    DomUtil::writePairListEntry(m_dom, m_configGroup, "envvar", "name", "value", list);
}

#include "environmentvariableswidget.moc"
