/* This file is part of KDevelop
    Copyright (C) 2003 Roberto Raggi <roberto@kdevelop.org>

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
    the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
    Boston, MA 02111-1307, USA.
*/

#ifndef CATALOG_H
#define CATALOG_H

#include "tag.h"

#include <qvaluelist.h>
#include <qpair.h>

class Catalog
{
public:
    typedef QPair<QString, QVariant> QueryArgument;

public:
    Catalog();
    virtual ~Catalog();

    bool isValid() const;
    QString dbName() const;

    virtual void open( const QString& dbName );
    virtual void close();
    virtual void sync();

    QStringList indexes() const;
    bool hasIndex( const QString& name ) const;
    void addIndex( const QString& name );
    void removeIndex( const QString& name );

    QString addItem( const Tag& tag );
    bool removeItem( const QString& id );

    Tag getItemById( const QString& id );
    QValueList<Tag> getAllItems();
    QValueList<Tag> query( const QValueList<QueryArgument>& args );

private:
    class Private;
    Private* d;
    
private:
    Catalog( const Catalog& source );
    void operator = ( const Catalog& source );	    
};

#endif
