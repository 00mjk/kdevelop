/* This file is part of KDevelop
Copyright 2001-2002 Matthias Hoelzer-Kluepfel <hoelzer@kde.org>
Copyright 2001-2002 Bernd Gehrmann <bernd@kdevelop.org>
Copyright 2001 Sandy Meier <smeier@kdevelop.org>
Copyright 2002 Daniel Engelschalt <daniel.engelschalt@gmx.net>
Copyright 2002 Simon Hausmann <hausmann@kde.org>
Copyright 2002-2003 Roberto Raggi <roberto@kdevelop.org>
Copyright 2003 Mario Scalas <mario.scalas@libero.it>
Copyright 2003 Harald Fernengel <harry@kdevelop.org>
Copyright 2003,2006,2008 Hamish Rodda <rodda@kde.org>
Copyright 2004 Alexander Dymo <adymo@kdevelop.org>
Copyright 2006 Adam Treat <treat@kde.org>

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

#include "codecontext.h"

namespace KDevelop
{

class CodeContext::Private
{
public:
    Private( const DUChainBasePointer& item ) : m_item( item )
    {}

    DUChainBasePointer m_item;
};

CodeContext::CodeContext( const DUChainBasePointer& item )
        : Context(), d( new Private( item ) )
{}

CodeContext::~CodeContext()
{
    delete d;
}

int CodeContext::type() const
{
    return Context::CodeContext;
}

const DUChainBasePointer& CodeContext::item() const
{
    return d->m_item;
}

}

