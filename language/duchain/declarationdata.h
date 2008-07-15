/* This file is part of KDevelop
    Copyright 2006 Hamish Rodda <rodda@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef DECLARATION_DATA_H
#define DECLARATION_DATA_H

#include "duchainbase.h"

#include "declaration.h"
#include "ducontext.h"
#include "topducontext.h"
#include "duchainlock.h"
#include "duchain.h"
#include "language/languageexport.h"
#include "language/duchain/types/indexedtype.h"

namespace KDevelop
{

class KDEVPLATFORMLANGUAGE_EXPORT DeclarationData : public DUChainBaseData
{
public:
  DeclarationData();

  DeclarationData( const DeclarationData& rhs );

  DUContext* m_context, *m_internalContext;
  IndexedType m_type;
  Identifier m_identifier;

  //Index in the comment repository
  uint m_comment;

  Declaration::Kind m_kind;

  bool m_isDefinition  : 1;
  bool m_inSymbolTable : 1;
  bool m_isTypeAlias   : 1;
  bool m_anonymousInContext : 1; //Whether the declaration was added into the parent-context anonymously

};

}

#endif
