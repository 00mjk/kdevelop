/* This file is part of KDevelop
    Copyright (C) 2005 Roberto Raggi <roberto@kdevelop.org>

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
    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
    Boston, MA 02110-1301, USA.
*/

#include "kdevtreeview.h"
#include "kdevitemmodel.h"

namespace KDevelop
{

TreeView::TreeView(QWidget *parent)
  : QTreeView(parent)
{
}

TreeView::~TreeView()
{
}

ItemModel *TreeView::itemModel() const
{
  return qobject_cast<ItemModel*>(model());
}

ItemCollection *TreeView::currentCollection() const
{
  Q_ASSERT(itemModel() != 0);

  QItemSelectionModel *selection = selectionModel();
  QModelIndex current = selection->currentIndex();

  while (current.isValid())
    {
      if (ItemCollection *collection = itemModel()->item(current)->collection())
        return collection;

      current = itemModel()->parent(current);
    }

  return 0;
}

ItemGroup *TreeView::currentGroup() const
{
  Q_ASSERT(itemModel() != 0);

  QItemSelectionModel *selection = selectionModel();
  QModelIndex current = selection->currentIndex();

  while (current.isValid())
    {
      if (ItemGroup *group = itemModel()->item(current)->group())
        return group;

      current = itemModel()->parent(current);
    }

  return 0;
}

Item *TreeView::currentItem() const
{
  Q_ASSERT(itemModel() != 0);

  QItemSelectionModel *selection = selectionModel();
  QModelIndex current = selection->currentIndex();

  while (current.isValid())
    {
      if (Item *item = itemModel()->item(current))
        return item;

      current = itemModel()->parent(current);
    }

  return 0;
}

}
#include "kdevtreeview.moc"
