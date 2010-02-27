/************************************************************************
 * KDevelop4 Custom Buildsystem Support                                 *
 *                                                                      *
 * Copyright 2010 Andreas Pakulat <apaku@gmx.de>                        *
 *                                                                      *
 * This program is free software; you can redistribute it and/or modify *
 * it under the terms of the GNU General Public License as published by *
 * the Free Software Foundation; either version 3 of the License, or    *
 * (at your option) any later version.                                  *
 *                                                                      *
 * This program is distributed in the hope that it will be useful, but  *
 * WITHOUT ANY WARRANTY; without even the implied warranty of           *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU     *
 * General Public License for more details.                             *
 *                                                                      *
 * You should have received a copy of the GNU General Public License    *
 * along with this program; if not, see <http://www.gnu.org/licenses/>. *
 ************************************************************************/

#include "includesmodel.h"

#include <klocale.h>

#include "custombuildsystemconfig.h"

IncludesModel::IncludesModel( QObject* parent )
    : QAbstractListModel( parent )
{
}

QVariant IncludesModel::data( const QModelIndex& index, int role ) const
{
    if( !index.isValid() || ( role != Qt::DisplayRole && role != Qt::EditRole ) ) {
        return QVariant();
    }

    if( index.row() < 0 || index.row() >= rowCount() || index.column() != 0 ) {
        return QVariant();
    }

    if( index.row() == m_includes.count() ) {
        return i18n( "Double Click here to insert a new include path" );
    } else {
        return m_includes.at( index.row() );
    }
}

int IncludesModel::rowCount( const QModelIndex& parent ) const
{
    if( parent.isValid() ) {
        return 0;
    }
    return m_includes.count() + 1;
}

bool IncludesModel::setData( const QModelIndex& index, const QVariant& value, int role )
{
    if( !index.isValid() || role != Qt::EditRole ) {
        return false;
    }
    if( index.row() < 0 || index.row() >= rowCount() || index.column() != 0 ) {
        return false;
    }

    if( index.row() == m_includes.count() ) {
        if( value.toString() != data( index ).toString() ) {
            beginInsertRows( QModelIndex(), m_includes.count(), m_includes.count() );
            m_includes << value.toString();
            endInsertRows();
        }
    } else {
        m_includes[index.row()] = value.toString();
        emit dataChanged( index, index );
        return true;
    }

    return false;
}

Qt::ItemFlags IncludesModel::flags( const QModelIndex& index ) const
{
    if( !index.isValid() ) {
        return 0;
    }

    return Qt::ItemFlags( Qt::ItemIsEditable | Qt::ItemIsSelectable | Qt::ItemIsEnabled );
}

QStringList IncludesModel::includes() const
{
    return m_includes;
}

void IncludesModel::setIncludes(const QStringList& includes )
{
    m_includes = includes;
    reset();
}

bool IncludesModel::removeRows( int row, int count, const QModelIndex& parent )
{
    if( row >= 0 && count > 0 && row < rowCount() - 1 ) {
        beginRemoveRows( parent, row, row + count - 1 );
        for( int i = row + count - 1; i >= row; i-- ) {
            m_includes.removeAt( i );
        }
        endRemoveRows();
    }
    return false;
}

#include "includesmodel.moc"
