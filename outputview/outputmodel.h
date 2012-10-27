/***************************************************************************
 *   This file is part of KDevelop                                         *
 *   Copyright 2007 Andreas Pakulat <apaku@gmx.de>                         *
 *   Copyright (C) 2012  Morten Danielsen Volden mvolden2@gmail.com        *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU Library General Public License as       *
 *   published by the Free Software Foundation; either version 2 of the    *
 *   License, or (at your option) any later version.                       *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU Library General Public     *
 *   License along with this program; if not, write to the                 *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.         *
 ***************************************************************************/

#ifndef OUTPUTMODEL_H
#define OUTPUTMODEL_H

#include "outputviewexport.h"
#include "ioutputviewmodel.h"
#include "filtereditem.h"
#include "ifilterstrategy.h"

#include <QtCore/QAbstractListModel>
#include <QString>
#include <KUrl>
#include <QLinkedList>
#include <QQueue>
#include <QSharedPointer>

#include <set>

namespace KDevelop
{

class FilteredItem;
class OutputModelPrivate;

class KDEVPLATFORMOUTPUTVIEW_EXPORT OutputModel : public QAbstractListModel, public KDevelop::IOutputViewModel
{
    Q_OBJECT
public:

    enum CustomRoles {
        OutputItemTypeRole = Qt::UserRole + 1
    };

    enum OutputFilterStrategy
    {
        NoFilter,
        CompilerFilter,
        ScriptErrorFilter,
        StaticAnalysisFilter
    };

    explicit OutputModel( const KUrl& builddir , QObject* parent = 0 );
    OutputModel( QObject* parent = 0 );
    virtual ~OutputModel();

    /// IOutputViewModel interfaces
    void activate( const QModelIndex& index );
    QModelIndex nextHighlightIndex( const QModelIndex &current );
    QModelIndex previousHighlightIndex( const QModelIndex &current );

    /// QAbstractItemModel interfaces
    QVariant data( const QModelIndex&, int = Qt::DisplayRole ) const;
    int rowCount( const QModelIndex& = QModelIndex() ) const;
    QVariant headerData( int, Qt::Orientation, int = Qt::DisplayRole ) const;

    void setFilteringStrategy(const OutputFilterStrategy& currentStrategy);
    void removeLastLines(int l);

    void flushLineBuffer();

public Q_SLOTS:
    void appendLine( const QString& );
    void appendLines( const QStringList& );

private slots:
    /// add batches of lines to prevent UI-lockup
    void addLineBatch();

private:
    OutputModelPrivate* const d;
};

//Q_DECLARE_METATYPE( OutputModel::OutputItemType )
}
#endif

