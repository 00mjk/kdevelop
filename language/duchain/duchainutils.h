/*
 * DUChain Utilities
 *
 * Copyright 2007 Hamish Rodda <rodda@kde.org>
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

#ifndef DUCHAINUTILS_H
#define DUCHAINUTILS_H

#include <KIcon>

#include <ktexteditor/codecompletionmodel.h>

#include "../languageexport.h"

class KUrl;

namespace KDevelop {

class Declaration;
class DUChainBase;
class DUContext;
class SimpleCursor;
class HashedString;
class TopDUContext;

/**
 * A namespace which contains convenience utilities for navigating definition-use chains.
 */
namespace DUChainUtils {
  KDEVPLATFORMLANGUAGE_EXPORT KTextEditor::CodeCompletionModel::CompletionProperties completionProperties(Declaration* dec);
  KDEVPLATFORMLANGUAGE_EXPORT QIcon iconForProperties(KTextEditor::CodeCompletionModel::CompletionProperties p);
  KDEVPLATFORMLANGUAGE_EXPORT QIcon iconForDeclaration(Declaration* dec);
  /** Asks the language-plugins for standard-contexts for the given url, and returns one if available.
    * If there is no language-plugin registered for the given url, it will just try to get any top-context for the file from the du-chain. */
  KDEVPLATFORMLANGUAGE_EXPORT KDevelop::TopDUContext* standardContextForUrl(const KUrl& url);
  /** Returns the Declaration/Definition under the cursor, or zero. DUChain does not need to be locked.
   * If the item under the cursor is a use, the declaration is returned. */
  KDEVPLATFORMLANGUAGE_EXPORT Declaration* itemUnderCursor(const KUrl& url, const SimpleCursor& cursor);
  /**If the given declaration is a definition, and has a real declaration
    *attached, returns that declarations. Else returns the given argument. */
  KDEVPLATFORMLANGUAGE_EXPORT Declaration* declarationForDefinition(Declaration* definition, TopDUContext* topContext = 0);
  ///Returns the first declaration in the given line. Searches the given context and all sub-contexts.
  KDEVPLATFORMLANGUAGE_EXPORT Declaration* declarationInLine(const KDevelop::SimpleCursor& cursor, KDevelop::DUContext* ctx);
  
  class KDEVPLATFORMLANGUAGE_EXPORT DUChainItemFilter {
    public:
    virtual bool accept(Declaration* decl) = 0;
    //Should return whether processing should be deepened into the given context
    virtual bool accept(DUContext* ctx) = 0;
    virtual ~DUChainItemFilter();
  };
  ///walks a context, all its sub-contexts, and all its declarations in exactly the order they appear in in the file.
  ///Re-implement DUChainItemFilter to do something with the items.
  KDEVPLATFORMLANGUAGE_EXPORT void collectItems( DUContext* context, DUChainItemFilter& filter );

  KDEVPLATFORMLANGUAGE_EXPORT DUContext* getArgumentContext(Declaration* decl);

}
}

#endif // DUCHAINUTILS_H
