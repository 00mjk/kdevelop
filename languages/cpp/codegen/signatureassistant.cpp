/*
   Copyright 2009 David Nolden <david.nolden.kdevelop@art-master.de>

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

#include "signatureassistant.h"

#include <interfaces/icore.h>
#include <interfaces/ilanguagecontroller.h>
#include <language/duchain/duchainutils.h>
#include <language/backgroundparser/backgroundparser.h>
#include <language/backgroundparser/parsejob.h>
#include <language/duchain/functiondefinition.h>
#include <language/duchain/types/functiontype.h>

#include <KTextEditor/Document>
#include <KTextEditor/View>
#include <KMessageBox>

#include "cppduchain.h"
#include "renameaction.h"

using namespace KDevelop;
using namespace Cpp;

Declaration *getDeclarationAtCursor(const SimpleCursor &cursor, const KUrl &documentUrl)
{
  ENSURE_CHAIN_READ_LOCKED
  ReferencedTopDUContext top(DUChainUtils::standardContextForUrl(documentUrl));
  if(!top)
    return 0;

  Declaration* functionDecl = DUChainUtils::declarationInLine(cursor, top.data());
  return functionDecl;
}

Signature getDeclarationSignature(const Declaration *functionDecl, const DUContext *functionCtxt)
{
  ENSURE_CHAIN_READ_LOCKED
  int pos = 0;
  Signature signature;
  const AbstractFunctionDeclaration* abstractFunDecl = dynamic_cast<const AbstractFunctionDeclaration*>(functionDecl);
  foreach(Declaration* parameter, functionCtxt->localDeclarations()) {
    signature.defaultParams << abstractFunDecl->defaultParameterForArgument(pos).str();
    signature.parameters << qMakePair(parameter->indexedType(), parameter->identifier().identifier().str());
    ++pos;
  }
  signature.isConst = functionDecl->abstractType() && functionDecl->abstractType()->modifiers() & AbstractType::ConstModifier;

  FunctionType::Ptr funType = functionDecl->type<FunctionType>();
  if(funType)
    signature.returnType = funType->returnType()->indexed();

  return signature;
}

AdaptDefinitionSignatureAssistant::AdaptDefinitionSignatureAssistant(KTextEditor::View* view, const KTextEditor::Range& inserted)
: m_editingDefinition(false), m_view(view)
{
  m_document = view->document()->url();

  DUChainReadLocker lock(DUChain::lock(), 300);
  if(!lock.locked()) {
    kDebug() << "failed to lock duchain in time";
    return;
  }

  SimpleRange invocationRange = SimpleRange(inserted);
  Declaration* funDecl = getDeclarationAtCursor(invocationRange.start, m_document);
  if(!funDecl || !funDecl->type<FunctionType>())
    return;

  Declaration* otherSide = 0;
  FunctionDefinition* definition = dynamic_cast<FunctionDefinition*>(funDecl);
  if (definition)
  {
    m_editingDefinition = true;
    otherSide = definition->declaration();
  }
  else if ((definition = FunctionDefinition::definition(funDecl)))
  {
    m_editingDefinition = false;
    otherSide = definition;
  }

  if (!otherSide)
    return;

  m_otherSideContext = DUContextPointer(DUChainUtils::getFunctionContext(otherSide));
  if (!m_otherSideContext)
    return;

  m_declarationName = funDecl->identifier();
  m_otherSideId = otherSide->id();
  m_otherSideTopContext = ReferencedTopDUContext(otherSide->topContext());
  m_oldSignature = getDeclarationSignature(otherSide, m_otherSideContext.data());

  connect(ICore::self()->languageController()->backgroundParser(), SIGNAL(parseJobFinished(KDevelop::ParseJob*)),
                                                                    SLOT(parseJobFinished(KDevelop::ParseJob*)));
  //Schedule an update, to make sure the ranges match
  DUChain::self()->updateContextForUrl(m_otherSideTopContext->url(), TopDUContext::AllDeclarationsAndContexts);
}

bool AdaptDefinitionSignatureAssistant::isUseful()
{
  return !m_declarationName.isEmpty() && m_otherSideId.isValid();
}

bool AdaptDefinitionSignatureAssistant::getSignatureChanges(const Signature& newSignature, QList< int >& oldPositions) const
{
  bool changed = false;
  for (int i = 0; i < newSignature.parameters.size(); ++i)
    oldPositions.append(-1);
  for (int curNewParam = newSignature.parameters.size() - 1; curNewParam >= 0 ; --curNewParam)
  {
    int foundAt = -1;

    for (int curOldParam = m_oldSignature.parameters.size() - 1; curOldParam >= 0 ; --curOldParam)
    {
      if (newSignature.parameters[curNewParam].first != m_oldSignature.parameters[curOldParam].first)
        continue; //Different type == different parameters

      if (newSignature.parameters[curNewParam].second == m_oldSignature.parameters[curOldParam].second || curOldParam == curNewParam)
      {
        //given the same type and either the same position or the same name, it's (probably) the same argument
        foundAt = curOldParam;

        if (newSignature.parameters[curNewParam].second != m_oldSignature.parameters[curOldParam].second || curOldParam != curNewParam)
          changed = true; //Either the name changed at this position, or position of this name has changed

        if (newSignature.parameters[curNewParam].second == m_oldSignature.parameters[curOldParam].second)
          break; //Found an argument with the same name and type, no need to look further
        //else: position/type match, but name match will trump, allowing: (int i=0, int j=1) => (int j=1, int i=0)
      }
    }

    if (foundAt < 0)
      changed = true;

    oldPositions[curNewParam] = foundAt;
  }

  if(newSignature.parameters.size() != m_oldSignature.parameters.size())
    changed = true;

  if(newSignature.isConst != m_oldSignature.isConst)
    changed = true;

  if(newSignature.returnType != m_oldSignature.returnType)
    changed = true;

  return changed;
}

void AdaptDefinitionSignatureAssistant::setDefaultParams(Signature& newSignature, const QList< int >& oldPositions) const
{
  for(int i = newSignature.parameters.size() - 1; i >= 0; --i)
  {
    if (oldPositions[i] == -1)
      return; //this param is new, no further defaults possible

    if (i == newSignature.defaultParams.size() - 1 || !newSignature.defaultParams[i+1].isEmpty())
      newSignature.defaultParams[i] = m_oldSignature.defaultParams[oldPositions[i]];
  }
}

QList< RenameAction* > AdaptDefinitionSignatureAssistant::getRenameActions(const Signature &newSignature, const QList<int> &oldPositions) const
{
  Q_ASSERT(DUChain::lock()->currentThreadHasReadLock());
  QList<RenameAction*> renameActions;
  if (!m_otherSideContext)
    return renameActions;

  for(int i = newSignature.parameters.size() - 1; i >= 0; --i)
  {
    if (oldPositions[i] == -1)
      continue; //new parameter

    Declaration *renamedDecl = m_otherSideContext->localDeclarations()[oldPositions[i]];
    if (newSignature.parameters[i].second != m_oldSignature.parameters[oldPositions[i]].second && renamedDecl->uses().size())
      renameActions << new RenameAction(renamedDecl->identifier(), newSignature.parameters[i].second, renamedDecl->uses());
  }
  return renameActions;
}

void AdaptDefinitionSignatureAssistant::parseJobFinished(KDevelop::ParseJob* job)
{
  if (job->document().toUrl() != m_document || !m_view)
    return;

  clearActions();

  DUChainReadLocker lock;

  Declaration *functionDecl = getDeclarationAtCursor(SimpleCursor(m_view->cursorPosition()), m_document);
  if (!functionDecl || functionDecl->identifier() != m_declarationName)
    return;
  DUContext *functionCtxt = DUChainUtils::getFunctionContext(functionDecl);
  if (!functionCtxt)
    return;
  //ParseJob having finished, get the signature that was modified
  Signature newSignature = getDeclarationSignature(functionDecl, functionCtxt);

  //Check for changes between m_oldSignature and newSignature, use oldPositions to store old<->new param index mapping
  QList<int> oldPositions;
  if (!getSignatureChanges(newSignature, oldPositions))
    return; //No changes to signature

  QList<RenameAction*> renameActions;
  if (m_editingDefinition)
    setDefaultParams(newSignature, oldPositions); //restore default parameters before updating the declarations
  else
    renameActions = getRenameActions(newSignature, oldPositions); //rename as needed when updating the definition
  addAction(IAssistantAction::Ptr(new AdaptSignatureAction(m_otherSideId, m_otherSideTopContext, m_oldSignature, newSignature, m_editingDefinition, renameActions)));

  emit actionsChanged();
}

#include "signatureassistant.moc"