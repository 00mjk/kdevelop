/*
 * KDevelop C++ Highlighting Support
 *
 * Copyright (c) 2006 Hamish Rodda <rodda@kde.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef KDEVCPPHIGHLIGHTING_H
#define KDEVCPPHIGHLIGHTING_H

#include <QObject>
#include <QHash>
#include <QModelIndex>

#include <ktexteditor/attribute.h>

#include "icodehighlighting.h"

namespace KTextEditor { class SmartRange; }

namespace KDevelop 
{
class DUContext;
class Declaration;
}

class CppHighlighting : public QObject, public KDevelop::ICodeHighlighting
{
  Q_OBJECT
  Q_INTERFACES(KDevelop::ICodeHighlighting)

  public:
    enum Types {
      UnknownType,
      ArgumentType,
      ClassType,
      CodeType,
      EnumType,
      EnumeratorType,
      FileType,
      FunctionDefinitionType,
      FunctionType,
      NamespaceType,
      ScopeType,
      TemplateType,
      TemplateParameterType,
      TypeAliasType,
      LocalVariableType,
      FunctionVariableType,
      MemberVariableType,
      NamespaceVariableType,
      GlobalVariableType,
      ErrorVariableType,
      ForwardDeclarationType
    };

    enum Contexts {
      DefinitionContext,
      DeclarationContext,
      ReferenceContext
    };

    CppHighlighting(QObject* parent);
    virtual ~CppHighlighting();

    void highlightTree(KTextEditor::SmartRange* topRange) const;
    void highlightDUChain(KDevelop::TopDUContext* context) const;

    virtual void highlightDefinition(KDevelop::Definition* definition) const;
    virtual void highlightDeclaration(KDevelop::Declaration* declaration) const;
    virtual void highlightUse(KDevelop::Use* use) const;

    KTextEditor::Attribute::Ptr attributeForType(Types type, Contexts context) const;
    KTextEditor::Attribute::Ptr attributeForDepth(int depth) const;

  private:
    void highlightDUChain(KDevelop::DUContext* context) const;
    void outputRange( KTextEditor::SmartRange * range ) const;

    Types typeForDeclaration(KDevelop::Declaration* dec) const;

    mutable QHash<Types, KTextEditor::Attribute::Ptr> m_definitionAttributes;
    mutable QHash<Types, KTextEditor::Attribute::Ptr> m_declarationAttributes;
    mutable QHash<Types, KTextEditor::Attribute::Ptr> m_referenceAttributes;

    mutable QList<KTextEditor::Attribute::Ptr> m_depthAttributes;
};

#endif

// kate: space-indent on; indent-width 2; replace-tabs on
