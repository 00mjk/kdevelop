/* This file is part of KDevelop
    Copyright 2008 Aleix Pol <aleixpol@gmail.com>

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

#include "projectproxymodel.h"
#include "projectmodel.h"
#include <KDebug>

ProjectProxyModel::ProjectProxyModel(QObject * parent)
    : QSortFilterProxyModel(parent)
{}

KDevelop::ProjectModel * ProjectProxyModel::projectModel() const
{
    return qobject_cast<KDevelop::ProjectModel*>( sourceModel() );
}

bool ProjectProxyModel::lessThan(const QModelIndex & left, const QModelIndex & right) const
{
    KDevelop::ProjectBaseItem *iLeft=projectModel()->item(left), *iRight=projectModel()->item(right);
    
    int leftType=iLeft->type(), rightType=iRight->type();
    bool ret;
    if(leftType==rightType)
    {
        QString strLeft=iLeft->text(), strRight=iRight->text();
        int extLeft=strLeft.lastIndexOf('.'), extRight=strRight.lastIndexOf('.');
        if(strLeft.right(strLeft.count()-extLeft)==strRight.right(strRight.count()-extRight))
            ret = strLeft > strRight;
        else
            ret = strLeft.right(strLeft.count()-extLeft)>strRight.right(strRight.count()-extRight);
    }
    else
    {
        ret = leftType > rightType;
    }
    return ret;
}

#include "projectproxymodel.moc"
