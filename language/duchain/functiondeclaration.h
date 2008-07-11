/* This file is part of KDevelop
    Copyright 2002-2005 Roberto Raggi <roberto@kdevelop.org>
    Copyright 2006 Adam Treat <treat@kde.org>
    Copyright 2006-2007 Hamish Rodda <rodda@kde.org>

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

#ifndef FUNCTIONDECLARATION_H
#define FUNCTIONDECLARATION_H

#include "language/duchain/declaration.h"
#include "language/duchain/abstractfunctiondeclaration.h"

namespace KDevelop
{
class FunctionDeclarationData;
/**
 * Represents a single variable definition in a definition-use chain.
 */
class KDEVPLATFORMLANGUAGE_EXPORT FunctionDeclaration : public Declaration, public AbstractFunctionDeclaration
{
public:
  FunctionDeclaration(const FunctionDeclaration& rhs);
  FunctionDeclaration(const SimpleRange& range, DUContext* context);
  FunctionDeclaration(FunctionDeclarationData& data);
  virtual ~FunctionDeclaration();

  virtual void setAbstractType(AbstractType::Ptr type);

  virtual Declaration* clone() const;

  virtual QString toString() const;

  virtual bool isFunctionDeclaration() const;
  
  virtual uint additionalIdentity() const;
  
  enum {
    Identity = 12
  };
  
  typedef Declaration Base;
private:
  DUCHAIN_DECLARE_DATA(FunctionDeclaration)
};
}

#endif // FUNCTIONDECLARATION_H

// kate: space-indent on; indent-width 2; tab-width 4; replace-tabs on; auto-insert-doxygen on
