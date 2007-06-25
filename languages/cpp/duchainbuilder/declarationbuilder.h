/* This file is part of KDevelop
    Copyright (C) 2006 Hamish Rodda <rodda@kde.org>

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

#ifndef DECLARATIONBUILDER_H
#define DECLARATIONBUILDER_H

#include "typebuilder.h"
#include "cppduchainbuilderexport.h"
#include <declaration.h>
#include <classfunctiondeclaration.h>

namespace KDevelop
{
class Declaration;
}

typedef TypeBuilder DeclarationBuilderBase;

/**
 * A class which iterates the AST to extract definitions of types.
 */
class KDEVCPPDUCHAINBUILDER_EXPORT DeclarationBuilder: public DeclarationBuilderBase
{
public:
  DeclarationBuilder(ParseSession* session);
  DeclarationBuilder(CppEditorIntegrator* editor);

  /**
   * Compile either a context-definition chain, or add uses to an existing
   * chain.
   *
   * \param includes contexts to reference from the top context.  The list may be changed by this function.
   */
  KDevelop::TopDUContext* buildDeclarations(const Cpp::LexedFilePointer& file, AST *node, QList<KDevelop::DUContext*>* includes = 0);
  
  /**
   * Build.an independent du-context based on a given parent-context. Such a context may be used for expression-parsing,
   * but should be deleted as fast as possible because it keeps a reference to an independent context.
   *
   * \param url A temporary url that can be used to identify this context
   *
   * \param parent Context that will be used as parent for this context
   */
  KDevelop::DUContext* buildSubDeclarations(const KUrl& url, AST *node, KDevelop::DUContext* parent = 0);

  protected:
  virtual void openContext(KDevelop::DUContext* newContext);
  virtual void closeContext();

  virtual void visitDeclarator (DeclaratorAST*);
  virtual void visitClassSpecifier(ClassSpecifierAST*);
  virtual void visitAccessSpecifier(AccessSpecifierAST*);
  virtual void visitFunctionDeclaration(FunctionDefinitionAST*);
  virtual void visitSimpleDeclaration(SimpleDeclarationAST*);
  virtual void visitElaboratedTypeSpecifier(ElaboratedTypeSpecifierAST*);
  virtual void visitParameterDeclaration(ParameterDeclarationAST* node);

  virtual void classTypeOpened(KDevelop::AbstractType::Ptr);
private:
  KDevelop::ForwardDeclaration* openForwardDeclaration(NameAST* name, AST* range);
  /**
   * Register a new declaration with the definition-use chain.
   * Returns the new context created by this definition.
   * \param range provide a valid AST here if name is null
   */
  KDevelop::Declaration* openDeclaration(NameAST* name, AST* range, bool isFunction = false, bool isForward = false, bool isDefinition = false);
  /// Same as the above, but sets it as the definition too
  KDevelop::Declaration* openDefinition(NameAST* name, AST* range, bool isFunction = false);
  void closeDeclaration();
  void abortDeclaration();

  void parseStorageSpecifiers(const ListNode<std::size_t>* storage_specifiers);
  void parseFunctionSpecifiers(const ListNode<std::size_t>* function_specifiers);

  inline bool hasCurrentDeclaration() const { return !m_declarationStack.isEmpty(); }
  inline KDevelop::Declaration* currentDeclaration() const { return m_declarationStack.top(); }

  template<class DeclarationType>
  inline DeclarationType* currentDeclaration() const { return dynamic_cast<DeclarationType*>(m_declarationStack.top()); }
  
  inline KDevelop::Declaration::AccessPolicy currentAccessPolicy() { return m_accessPolicyStack.top(); }
  inline void setAccessPolicy(KDevelop::Declaration::AccessPolicy policy) { m_accessPolicyStack.top() = policy; }

  inline int& nextDeclaration() { return m_nextDeclarationStack.top(); }

  void applyStorageSpecifiers();
  void applyFunctionSpecifiers();
  void popSpecifiers();

  QStack<KDevelop::Declaration*> m_declarationStack;
  QStack<KDevelop::Declaration::AccessPolicy> m_accessPolicyStack;
  QStack<int> m_nextDeclarationStack;

  QStack<KDevelop::ClassFunctionDeclaration::FunctionSpecifiers> m_functionSpecifiers;
  QStack<KDevelop::ClassMemberDeclaration::StorageSpecifiers> m_storageSpecifiers;
  QStack<std::size_t> m_functionDefinedStack;
};

#endif // DECLARATIONBUILDER_H

// kate: indent-width 2;
