/* This file is part of the KDE project
   Copyright (C) 2006 Hamish Rodda <rodda@kde.org>

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
#ifndef KDEVCODEHIGHLIGHTING_H
#define KDEVCODEHIGHLIGHTING_H

#include "kdevexport.h"

class TopDUContext;
class Definition;
class Declaration;
class Use;

namespace KDevelop
{

/**
KDevelop text highlighting support interface - the base class for
language support text highlighting routines. Languages can apply highlighting
to parsed code here.
*/
class KDEVPLATFORM_EXPORT CodeHighlighting
{
public:
    virtual ~CodeHighlighting() {}

    /**@return Whether the given url is supported by the language part.*/
    virtual void highlightDUChain(TopDUContext* context) const = 0;

    virtual void highlightDefinition(Definition* definition) const = 0;
    virtual void highlightDeclaration(Declaration* declaration) const = 0;
    virtual void highlightUse(Use* use) const = 0;
};

}
#endif
