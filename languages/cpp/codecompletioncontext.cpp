/*
   Copyright 2007 David Nolden <david.nolden.kdevelop@art-master.de>

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

#include "codecompletioncontext.h"
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <klocalizedstring.h>
#include <language/duchain/ducontext.h>
#include <language/duchain/duchain.h>
#include <language/duchain/namespacealiasdeclaration.h>
#include <language/duchain/classfunctiondeclaration.h>
#include <language/duchain/functiondefinition.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/stringhelpers.h>
#include <language/duchain/safetycounter.h>
#include <language/interfaces/iproblem.h>
#include <util/pushvalue.h>
#include "cppduchain/cppduchain.h"
#include "cppduchain/typeutils.h"
#include "cppduchain/overloadresolution.h"
#include "cppduchain/viablefunctions.h"
#include "cppduchain/environmentmanager.h"
#include "cpptypes.h"
#include "stringhelpers.h"
#include "templatedeclaration.h"
#include "cpplanguagesupport.h"
#include "environmentmanager.h"
#include "cppduchain/cppduchain.h"
#include "cppdebughelper.h"
#include "missingincludecompletionitem.h"

#define LOCKDUCHAIN     DUChainReadLocker lock(DUChain::lock())
#define DEBUG
#define ifDebug(x) x
//Whether the list of argument-hints should contain all overloaded versions of operators.
//Disabled for now, because there is usually a huge list of overloaded operators.
const int maxOverloadedOperatorArgumentHints = 5;
const int maxOverloadedArgumentHints = 5;

using namespace KDevelop;

namespace Cpp {

///@todo move these together with those from expressionvisitor into an own file, or make them unnecessary
QList<DeclarationPointer> convert( const QList<Declaration*>& list ) {
  QList<DeclarationPointer> ret;
  foreach( Declaration* decl, list )
    ret << DeclarationPointer(decl);
  return ret;
}

QList<Declaration*> convert( const QList<DeclarationPointer>& list ) {
  QList<Declaration*> ret;
  foreach( DeclarationPointer decl, list )
    if( decl )
      ret << decl.data();
  return ret;
}

QList<Declaration*> convert( const DeclarationId* decls, uint count, TopDUContext* top ) {
  QList<Declaration*> ret;
  for(int a = 0; a < count; ++a) {
    Declaration* d = decls[a].getDeclaration(top);
    if(d)
      ret << d;
  }
    
  return ret;
}

typedef PushValue<int> IntPusher;

///Extracts the last line from the given string
QString extractLastLine(const QString& str) {
  int prevLineEnd = str.lastIndexOf('\n');
  if(prevLineEnd != -1)
    return str.mid(prevLineEnd+1);
  else
    return str;
}

bool isKeyword(QString str) {
  ///@todo Complete this list
  return str == "new" || str == "return" || str == "else" || str == "throw";
}

int completionRecursionDepth = 0;

CodeCompletionContext::CodeCompletionContext(DUContextPointer context, const QString& text, const QString& followingText, int depth, const QStringList& knownArgumentExpressions, int line ) : KDevelop::CodeCompletionContext(context, text, depth), m_memberAccessOperation(NoMemberAccess), m_knownArgumentExpressions(knownArgumentExpressions), m_contextType(Normal)
{
  m_followingText = followingText;
  
  if(depth > 10) {
    kDebug() << "too much recursion";
    m_valid = false;
    return;
  }
  
  {
    //Since include-file completion has nothing in common with the other completion-types, process it within a separate function
    QString lineText = extractLastLine(m_text).trimmed();
    if(lineText.startsWith("#include")) {
      processIncludeDirective(lineText);
      return;
    }
  }
  
  m_valid = isValidPosition();
  if( !m_valid ) {
    log( "position not valid for code-completion" );
    return;
  }

  ifDebug( log( "non-processed text: " + m_text ); )
   preprocessText( line );

   m_text = Utils::clearComments( m_text );
   m_text = Utils::clearStrings( m_text );
   m_text = Utils::stripFinalWhitespace( m_text );

  ifDebug( log( "processed text: " + m_text ); )

  ///@todo template-parameters

  ///First: find out what kind of completion we are dealing with

  if( (m_text.endsWith( ':' ) && !m_text.endsWith("::")) || m_text.endsWith( ';' ) || m_text.endsWith('}') || m_text.endsWith('{') || m_text.endsWith(')') ) {
    ///We're at the beginning of a new statement. General completion is valid.
    return;
  }

    if( m_text.endsWith('.') ) {
    m_memberAccessOperation = MemberAccess;
    m_text = m_text.left( m_text.length()-1 );
  }

  if( m_text.endsWith("->") ) {
    m_memberAccessOperation = ArrowMemberAccess;
    m_text = m_text.left( m_text.length()-2 );
  }

  if( m_text.endsWith("::") ) {
    m_memberAccessOperation = StaticMemberChoose; //We need to decide later whether it is a MemberChoose
    m_text = m_text.left( m_text.length()-2 );
  }

  if( endsWithOperator( m_text ) ) {
    if( depth == 0 ) {
      //The first context should never be a function-call context, so make this a NoMemberAccess context and the parent a function-call context.
      m_parentContext = new CodeCompletionContext( m_duContext, m_text, QString(), depth+1 );
      return;
    }
    m_memberAccessOperation = FunctionCallAccess;
    m_contextType = BinaryOperatorFunctionCall;
    m_operator = getEndFunctionOperator(m_text);
    m_text = m_text.left( m_text.length() - getEndOperator(m_text).length() );
  }

  if( m_text.endsWith('(') ) {
    if( depth == 0 ) {
      //The first context should never be a function-call context, so make this a NoMemberAccess context and the parent a function-call context.
      m_parentContext = new CodeCompletionContext( m_duContext, m_text, QString(), depth+1 );
      return;
    }
    m_contextType = FunctionCall;
    m_memberAccessOperation = FunctionCallAccess;
    m_text = m_text.left( m_text.length()-1 );

    ///Compute the types of the argument-expressions
    ExpressionParser expressionParser;

    for( QStringList::const_iterator it = m_knownArgumentExpressions.begin(); it != m_knownArgumentExpressions.end(); ++it )
      m_knownArgumentTypes << expressionParser.evaluateExpression( (*it).toUtf8(), m_duContext );
  }

  ///Now find out where the expression starts

  /**
   * Possible cases:
   * a = exp; - partially handled
   * ...
   * return exp;
   * emit exp;
   * throw exp;
   * new Class;
   * a=function(exp
   * a = exp(
   * ClassType instance(
   *
   * What else?
   *
   * When the left and right part are only separated by a whitespace,
   * expressionAt returns both sides
   * */

  int start_expr = Utils::expressionAt( m_text, m_text.length() );

  m_expression = m_text.mid(start_expr).trimmed();

  if(m_expression == "else")
    m_expression = QString();
  
  QString expressionPrefix = Utils::stripFinalWhitespace( m_text.left(start_expr) );

  ifDebug( log( "expressionPrefix: " + expressionPrefix ); )

  ///Handle constructions like "ClassType instance("
  if(!expressionPrefix.isEmpty() && (expressionPrefix.endsWith('>') || expressionPrefix[expressionPrefix.length()-1].isLetterOrNumber() || expressionPrefix[expressionPrefix.length()-1] == '_')) {
    int newExpressionStart = Utils::expressionAt(expressionPrefix, expressionPrefix.length());
    if(newExpressionStart > 0) {
      QString newExpression = expressionPrefix.mid(newExpressionStart).trimmed();
      QString newExpressionPrefix = Utils::stripFinalWhitespace( expressionPrefix.left(newExpressionStart) );
      if(!isKeyword(newExpression)) {
        if(newExpressionPrefix.isEmpty() || newExpressionPrefix.endsWith(';') || newExpressionPrefix.endsWith('{') || newExpressionPrefix.endsWith('}')) {
          kDebug(9007) << "skipping expression" << m_expression << "and setting new expression" << newExpression;
          m_expression = newExpression;
          expressionPrefix = newExpressionPrefix;
        }
      }
    }
  }

  ///Handle recursive contexts(Example: "ret = function1(param1, function2(" )
  if( expressionPrefix.endsWith('(') || expressionPrefix.endsWith(',') ) {
    log( QString("Recursive function-call: Searching parent-context in \"%1\"").arg(expressionPrefix) );
    //Our expression is within a function-call. We need to find out the possible argument-types we need to match, and show an argument-hint.

    //Find out which argument-number this expression is, and compute the beginning of the parent function-call(parentContextLast)
    QStringList otherArguments;
    int parentContextEnd = expressionPrefix.length();
    
    skipFunctionArguments( expressionPrefix, otherArguments, parentContextEnd );

    QString parentContextText = expressionPrefix.left(parentContextEnd);

    log( QString("This argument-number: %1 Building parent-context from \"%2\"").arg(otherArguments.size()).arg(parentContextText) );
    m_parentContext = new CodeCompletionContext( m_duContext, parentContextText, QString(), depth+1, otherArguments );
  }

  ///Handle overridden binary operator-functions
  if( endsWithOperator(expressionPrefix) ) {
    log( QString( "Recursive operator: creating parent-context with \"%1\"" ).arg(expressionPrefix) );
    m_parentContext = new CodeCompletionContext( m_duContext, expressionPrefix, QString(), depth+1 );
  }

  ///Now care about m_expression. It may still contain keywords like "new "

  bool isEmit = false, isReturn = false, isThrow = false;

  QString expr = m_expression.trimmed();

  if( expr.startsWith("emit") )  {
    isEmit = true; //When isEmit is true, we should filter the result so only signals are left
    expr = expr.right( expr.length() - 4 );
  }
  if( expr.startsWith("return") )  {
    isReturn = true; //When isReturn is true, we should match the result against the return-type of the current context-function
    expr = expr.right( expr.length() - 6 );
  }
  if( expr.startsWith("throw") )  {
    isThrow = true;
    expr = expr.right( expr.length() - 5 );
  }

  ExpressionParser expressionParser/*(false, true)*/;

  ifDebug( kDebug(9007) << "expression: " << expr; )
  
  if( !expr.trimmed().isEmpty() ) {
    m_expressionResult = expressionParser.evaluateExpression( expr.toUtf8(), m_duContext );
    ifDebug( kDebug(9007) << "expression result: " << m_expressionResult.toString(); )
    if( !m_expressionResult.isValid() ) {
      if( m_memberAccessOperation != StaticMemberChoose ) {
        log( QString("expression \"%1\" could not be evaluated").arg(expr) );
        m_valid = false;
        return;
      } else {
        //It may be an access to a namespace, like "MyNamespace::".
        //The "MyNamespace" can not be evaluated, still we can give some completions.
        return;
      }
    }
  }

  switch( m_memberAccessOperation ) {

    case NoMemberAccess:
    {
      if( !expr.trimmed().isEmpty() ) {
        //This should never happen, because the position-cursor should be chosen at the beginning of a possible completion-context(not in the middle of a string)
        log( QString("Cannot complete \"%1\" because there is an expression without an access-operation" ).arg(expr) );
        m_valid  = false;
      } else {
        //Do nothing. We do not have a completion-container, which means that a global completion should be done.
      }
    }
    break;
    case MemberChoose:
    case StaticMemberChoose:
    {
      ///@todo Check whether it is a MemberChoose
    }
    case ArrowMemberAccess:
    {
      LOCKDUCHAIN;
      //Dereference a pointer
      AbstractType::Ptr containerType = m_expressionResult.type.type();
      CppPointerType::Ptr pnt = TypeUtils::realType(containerType, m_duContext->topContext()).cast<CppPointerType>();
      if( !pnt ) {
        AbstractType::Ptr realContainer = TypeUtils::realType(containerType, m_duContext->topContext());
        IdentifiedType* idType = dynamic_cast<IdentifiedType*>(realContainer.unsafeData());
        if( idType ) {
          Declaration* idDecl = idType->declaration(m_duContext->topContext());
          if( idDecl && idDecl->internalContext() ) {
            QList<Declaration*> operatorDeclarations = idDecl->internalContext()->findLocalDeclarations(Identifier("operator->"));
            if( !operatorDeclarations.isEmpty() ) {
              ///@todo care about const
              foreach(Declaration* decl, operatorDeclarations)
                m_expressionResult.allDeclarationsList().append(decl->id());
              
              CppFunctionType::Ptr function = operatorDeclarations.front()->abstractType().cast<CppFunctionType>();

              if( function ) {
                m_expressionResult.type = function->returnType()->indexed();
                m_expressionResult.isInstance = true;
              } else {
                  log( QString("arrow-operator of class is not a function: %1").arg(containerType ? containerType->toString() : QString("null") ) );
              }
            } else {
              log( QString("arrow-operator on type without operator* member: %1").arg(containerType ? containerType->toString() : QString("null") ) );
            }
          } else {
            log( QString("arrow-operator on type without declaration and context: %1").arg(containerType ? containerType->toString() : QString("null") ) );
          }
        } else {
          log( QString("arrow-operator on invalid type: %1").arg(containerType ? containerType->toString() : QString("null") ) );
          m_expressionResult = ExpressionEvaluationResult();
        }
      }

      if( pnt ) {
        ///@todo what about const in pointer?
        m_expressionResult.type = pnt->baseType()->indexed();
        m_expressionResult.isInstance = true;
      }
    }
    case MemberAccess:
    {
      if( expr.trimmed().isEmpty() ) {
        log( "Expression was empty, cannot complete" );
        m_valid = false;
      }

      //The result of the expression is stored in m_expressionResult, so we're fine
      
      ///Additional step: Check whether we're accessing a declaration that is not available, and eventually allow automatically adding an #include
      LOCKDUCHAIN;
      AbstractType::Ptr type = m_expressionResult.type.type();
      if(type && m_duContext) {
        DelayedType::Ptr delayed = type.cast<DelayedType>();
#ifndef TEST_COMPLETION
        if(delayed && delayed->kind() == DelayedType::Unresolved)
          m_storedItems += missingIncludeCompletionItems(m_expression, m_followingText.trimmed() + ": ", m_expressionResult, m_duContext.data());
#endif
      }else{
        log( "No type for expression" );
      }
    }
    break;
    case FunctionCallAccess:
      processFunctionCallAccess();
    break;
  }
}

CodeCompletionContext::AdditionalContextType CodeCompletionContext::additionalContextType() const {
  return m_contextType;
}

void CodeCompletionContext::processFunctionCallAccess() {
  ///Generate a list of all found functions/operators, together with each a list of optional prefixed parameters

  ///All the variable argument-count management in the following code is done to treat global operator-functions equivalently to local ones. Those take an additional first argument.

  LOCKDUCHAIN;
  
  OverloadResolutionHelper helper( m_duContext, TopDUContextPointer(m_duContext->topContext()) );
  
  if( m_contextType == BinaryOperatorFunctionCall ) {

    if( !m_expressionResult.isInstance ) {
      log( "tried to apply an operator to a non-instance: " + m_expressionResult.toString() );
      m_valid = false;
      return;
    }

    helper.setOperator(OverloadResolver::Parameter(m_expressionResult.type.type(), m_expressionResult.isLValue()), m_operator);
    
    m_functionName = "operator"+m_operator;
  } else {
    ///Simply take all the declarations that were found by the expression-parser
    
    helper.setFunctions(convert(m_expressionResult.allDeclarations(), m_expressionResult.allDeclarationsSize(), m_duContext->topContext()));
    
    if(m_expressionResult.allDeclarationsSize()) {
      Declaration* decl = m_expressionResult.allDeclarations()[0].getDeclaration(m_duContext->topContext());
      if(decl)
        m_functionName = decl->identifier().toString();
    }
  }

  OverloadResolver::ParameterList knownParameters;
  foreach( ExpressionEvaluationResult result, m_knownArgumentTypes )
    knownParameters.parameters << OverloadResolver::Parameter( result.type.type(), result.isLValue() );
  
  helper.setKnownParameters(knownParameters);
  
  m_functions = helper.resolve(true);

//   if( declarations.isEmpty() ) {
//     log( QString("no list of function-declarations was computed for expression \"%1\"").arg(m_expression) );
//     return;
//   }
}

void CodeCompletionContext::processIncludeDirective(QString line)
{
  if(line.count('"') == 2 || line.endsWith('>'))
    return; //We are behind a complete include-directive

  kDebug(9007) << "include line: " << line;
  line = line.mid(8).trimmed(); //Strip away the #include
  kDebug(9007) << "trimmed include line: " << line;

  if(!line.startsWith('<') && !line.startsWith('"'))
    return; //We are not behind the beginning of a path-specification

  bool local = false;
  if(line.startsWith('"'))
    local = true;
  
  line = line.mid(1);

  kDebug(9007) << "extract prefix from " << line;
  //Extract the prefix-path
  KUrl u(line);
  u.setFileName(QString());
  QString prefixPath = u.path();
  kDebug(9007) << "extracted prefix " << prefixPath;
  
  LOCKDUCHAIN;
  if(!m_duContext)
    return;
#ifndef TEST_COMPLETION
  m_includeItems = CppLanguageSupport::self()->allFilesInIncludePath(KUrl(m_duContext->url().str()), local, prefixPath);
#endif
  m_valid = true;
  m_memberAccessOperation = IncludeListAccess;
}

const CodeCompletionContext::FunctionList& CodeCompletionContext::functions() const {
  return m_functions;
}

QString CodeCompletionContext::functionName() const {
  return m_functionName;
}

QList<Cpp::IncludeItem> CodeCompletionContext::includeItems() const {
  return m_includeItems;
}

ExpressionEvaluationResult CodeCompletionContext::memberAccessContainer() const {
  return m_expressionResult;
}

QList<DUContext*> CodeCompletionContext::memberAccessContainers() const {
  QList<DUContext*> ret;
  
  if( memberAccessOperation() == StaticMemberChoose && m_duContext ) {
    //Locate all namespace-instances we will be completing from
    ret += m_duContext->findContexts(DUContext::Class, QualifiedIdentifier(m_expression)); 
    ret += m_duContext->findContexts(DUContext::Namespace, QualifiedIdentifier(m_expression)); ///@todo respect position
  }

  if(m_expressionResult.isValid() ) {
    AbstractType::Ptr expressionTarget = TypeUtils::targetType(m_expressionResult.type.type(), m_duContext->topContext());
    const IdentifiedType* idType = dynamic_cast<const IdentifiedType*>( expressionTarget.unsafeData() );
      Declaration* idDecl = 0;
    if( idType && (idDecl = idType->declaration(m_duContext->topContext())) ) {
      DUContext* ctx = idDecl->logicalInternalContext(m_duContext->topContext());
      if( ctx ){
        ret << ctx;
      }else {
        //Print some debug-output
        kDebug(9007) << "Could not get internal context from" << m_expressionResult.type.type()->toString();
        kDebug(9007) << "Declaration" << idDecl->toString() << idDecl->isForwardDeclaration();
        if( Cpp::TemplateDeclaration* tempDeclaration = dynamic_cast<Cpp::TemplateDeclaration*>(idDecl) ) {
          if( tempDeclaration->instantiatedFrom() ) {
            kDebug(9007) << "instantiated from" << dynamic_cast<Declaration*>(tempDeclaration->instantiatedFrom())->toString() << dynamic_cast<Declaration*>(tempDeclaration->instantiatedFrom())->isForwardDeclaration();
            kDebug(9007) << "internal context" << dynamic_cast<Declaration*>(tempDeclaration->instantiatedFrom())->internalContext();
          }
        }
        
      }
    }
  }
  
  return ret;
}

CodeCompletionContext::~CodeCompletionContext() {
}

bool CodeCompletionContext::isValidPosition() {
  if( m_text.isEmpty() )
    return true;
  //If we are in a string or comment, we should not complete anything
  QString markedText = Utils::clearComments(m_text, '$');
  markedText = Utils::clearStrings(markedText,'$');

  if( markedText[markedText.length()-1] == '$' ) {
    //We are within a comment or string
    kDebug(9007) << "code-completion position is invalid, marked text: \n\"" << markedText << "\"\n unmarked text:\n" << m_text << "\n";
    return false;
  }
  return true;
}

QString originalOperator( const QString& str ) {
  if( str == "[" )
    return "[]";
  return str;
}

QString CodeCompletionContext::getEndOperator( const QString& str ) const {
  static QStringList allowedOperators = QString("++ + -- += -= *= /= %= ^= &= |= << >> >>= <<= == != <= >= && || [ - * / % & | = < >" ).split( ' ', QString::SkipEmptyParts );

  for( QStringList::const_iterator it = allowedOperators.begin(); it != allowedOperators.end(); ++it )
    if( str.endsWith(*it) )
      return *it;
  return QString();
}

QString CodeCompletionContext::getEndFunctionOperator( const QString& str ) const {
  return originalOperator( getEndOperator( str ) );
}

bool CodeCompletionContext::endsWithOperator( const QString& str ) const {
  return !getEndOperator(str).isEmpty();
}

QList<KDevelop::AbstractType::Ptr> CodeCompletionContext::additionalMatchTypes() const {
  QList<KDevelop::AbstractType::Ptr> ret;
  if( m_operator == "=" && m_expressionResult.isValid() && m_expressionResult.isInstance ) {
    //Conversion to the left operand-type
    ret << m_expressionResult.type.type();
  }
  return ret;
}

void CodeCompletionContext::preprocessText( int line ) {
  
  LOCKDUCHAIN;
  
  if( m_duContext ) {
  m_text = preprocess( m_text,  dynamic_cast<Cpp::EnvironmentFile*>(m_duContext->topContext()->parsingEnvironmentFile().data()), line );
  }else{
    kWarning() << "error: no ducontext";
  }
}

CodeCompletionContext::MemberAccessOperation CodeCompletionContext::memberAccessOperation() const {
  return m_memberAccessOperation;
}

CodeCompletionContext* CodeCompletionContext::parentContext() {
  return static_cast<CodeCompletionContext*>(KDevelop::CodeCompletionContext::parentContext());
}

#ifndef TEST_COMPLETION

QList<CompletionTreeItemPointer> CodeCompletionContext::completionItems(const KDevelop::SimpleCursor& position, bool& shouldAbort) {
    LOCKDUCHAIN;
    
    QList<CompletionTreeItemPointer> items;
    
    if(!m_duContext)
      return items;
    
    typedef QPair<Declaration*, int> DeclarationDepthPair;
    
    if(!m_storedItems.isEmpty()) {
      items = m_storedItems;
    }else{
      if( memberAccessContainer().isValid() ||memberAccessOperation() == Cpp::CodeCompletionContext::StaticMemberChoose )
      {
        QList<DUContext*> containers = memberAccessContainers();
        if( !containers.isEmpty() ) {
          QSet<DUContext*> had;
          foreach(DUContext* ctx, containers) {
            if(had.contains(ctx)) ///@todo why do we need this
              continue;
            had.insert(ctx);
            
            if (shouldAbort)
              return items;
            
            foreach( const DeclarationDepthPair& decl, Cpp::hideOverloadedDeclarations( ctx->allDeclarations(ctx->range().end, m_duContext->topContext(), false ) ) ) {
              //If we have StaticMemberChoose, which means A::Bla, show only static members, except if we're within a class that derives from the container
              if(memberAccessOperation() != Cpp::CodeCompletionContext::StaticMemberChoose) {
                if(decl.first->kind() != Declaration::Instance)
                  continue;
                ClassMemberDeclaration* classMember = dynamic_cast<ClassMemberDeclaration*>(decl.first);
                if(classMember && classMember->isStatic())
                  continue; //Skip static class members when not doing static access
                if(decl.first->abstractType().cast<CppEnumeratorType>())
                  continue; //Skip enumerators
              }else{
                ///@todo what NOT to show on static member choose? Actually we cannot hide all non-static functions, because of function-pointers
              }
              
              if(!decl.first->identifier().isEmpty())
                items << CompletionTreeItemPointer( new NormalDeclarationCompletionItem( DeclarationPointer(decl.first), CodeCompletionContext::Ptr(this), decl.second ) );
            }
          }
        } else {
          kDebug(9007) << "setContext: no container-type";
        }
      } else if( memberAccessOperation() == Cpp::CodeCompletionContext::IncludeListAccess ) {
        //Include-file completion
        int cnt = 0;
        QList<Cpp::IncludeItem> allIncludeItems = includeItems();
        foreach(const Cpp::IncludeItem& includeItem, allIncludeItems) {
          if (shouldAbort)
            return items;

          items << CompletionTreeItemPointer( new IncludeFileCompletionItem(includeItem) );
          ++cnt;
        }
        kDebug(9007) << "Added " << cnt << " include-files to completion-list";
      } else {
        standardAccessCompletionItems(position, items);

        //kDebug(9007) << "setContext: using all declarations visible:" << decls.size();
      }
    }


    int argumentHintDepth = 0;
    ///Find all recursive function-calls that should be shown as call-tips
    CodeCompletionContext::Ptr parentContext(this);
    do {
      if (shouldAbort)
        return items;

      ++argumentHintDepth;
      
      parentContext = parentContext->parentContext();
      if( parentContext ) {
        if( parentContext->memberAccessOperation() == Cpp::CodeCompletionContext::FunctionCallAccess ) {
          //When there is too many overloaded functions, do not show them. They can just be too many.
          if( ((parentContext->functions().count() == 0 || parentContext->functions().count() > maxOverloadedOperatorArgumentHints) && parentContext->additionalContextType() == Cpp::CodeCompletionContext::BinaryOperatorFunctionCall)
                || parentContext->functions().count() > maxOverloadedArgumentHints) {
            items << CompletionTreeItemPointer( new NormalDeclarationCompletionItem( KDevelop::DeclarationPointer(), parentContext, 0, 0 ) );
            if(parentContext->functions().count())
              items.back()->asItem<NormalDeclarationCompletionItem>()->alternativeText = i18n("%1 overloads of", parentContext->functions().count()) + " " + parentContext->functionName();
            else
              items.back()->asItem<NormalDeclarationCompletionItem>()->alternativeText = parentContext->functionName();
          }else if(parentContext->functions().count() == 0) {
            items += missingIncludeCompletionItems(parentContext->m_expression, QString(), parentContext->m_expressionResult, m_duContext.data(), argumentHintDepth );
          }else{
            int num = 0;
            foreach( Cpp::CodeCompletionContext::Function function, parentContext->functions() ) {
              items << CompletionTreeItemPointer( new NormalDeclarationCompletionItem( function.function.declaration(), parentContext, 0, num ) );
              ++num;
            }
          }
        } else {
          kDebug(9007) << "parent-context has non function-call access type";
        }
      }
    } while( parentContext );
    
  //Eventually add missing include-completion in cases like SomeNamespace::NotIncludedClass|
  if(memberAccessOperation() == StaticMemberChoose && !m_followingText.trimmed().isEmpty()) {
    QString totalExpression = m_expression + "::" + m_followingText.trimmed();
    items += missingIncludeCompletionItems(totalExpression, m_followingText.trimmed() + ": ", ExpressionEvaluationResult(), m_duContext.data(), 0);
  }
  
  return items;
}

QualifiedIdentifier CodeCompletionContext::requiredPrefix(Declaration* decl) const {
  QualifiedIdentifier worstCase = decl->context()->scopeIdentifier(true);
  if(!m_duContext)
    return worstCase;
  QualifiedIdentifier currentPrefix;

  while(1) {
    QList<Declaration*> found = m_duContext->findDeclarations( currentPrefix + decl->identifier() );
    if(found.contains(decl))
      return currentPrefix;
  
    if(currentPrefix.count() >= worstCase.count()) {
      return worstCase;
    }else {
      currentPrefix.push(worstCase.at(currentPrefix.count()));
    }
  }
}

void CodeCompletionContext::standardAccessCompletionItems(const KDevelop::SimpleCursor& position, QList<CompletionTreeItemPointer>& items) {
  //Normal case: Show all visible declarations
  typedef QPair<Declaration*, int> DeclarationDepthPair;
  
  QList<DeclarationDepthPair> decls = m_duContext->allDeclarations(m_duContext->type() == DUContext::Class ? m_duContext->range().end : position, m_duContext->topContext());
  
  if(m_duContext) {
    //Collect the Declarations from all "using namespace" imported contexts
    QList<Declaration*> imports = m_duContext->findDeclarations( globalImportIdentifier, position );
    QSet<QualifiedIdentifier> ids;
    foreach(Declaration* importDecl, imports) {
      NamespaceAliasDeclaration* aliasDecl = dynamic_cast<NamespaceAliasDeclaration*>(importDecl);
      if(aliasDecl) {
        ids.insert(aliasDecl->importIdentifier());
      }else{
        kDebug() << "Import is not based on NamespaceAliasDeclaration";
      }
    }
    
    foreach(QualifiedIdentifier id, ids) {
      QList<DUContext*> importedContexts = m_duContext->findContexts( DUContext::Namespace, id );
      foreach(DUContext* context, importedContexts)
        foreach(Declaration* decl, context->localDeclarations())
          decls << qMakePair(decl, 1200);
    }
  }

  QList<DeclarationDepthPair> oldDecls = decls;
  decls.clear();

  //Remove pure function-definitions before doing overload-resolution, so they don't hide their own declarations.
  foreach( const DeclarationDepthPair& decl, oldDecls )
    if(!dynamic_cast<FunctionDefinition*>(decl.first) || !static_cast<FunctionDefinition*>(decl.first)->hasDeclaration())
      decls << decl;
  
  decls = Cpp::hideOverloadedDeclarations(decls);
  
  foreach( const DeclarationDepthPair& decl, decls )
    items << CompletionTreeItemPointer( new NormalDeclarationCompletionItem(DeclarationPointer(decl.first), Ptr(this), decl.second ) );
  
  CodeCompletionContext* parent = parentContext();
  if(parent) {
    if(parent->memberAccessOperation() == FunctionCallAccess) {
        foreach(const Cpp::OverloadResolutionFunction& function, parent->functions()) {
          if(function.function.isValid() && function.function.isViable() && function.function.declaration()) {
            //uint parameterNumber = parent->m_knownArgumentExpressions.size() + function.matchedArguments;
            Declaration* functionDecl = function.function.declaration().data();
            if(functionDecl->type<CppFunctionType>()->arguments().count() > function.matchedArguments) {
              //Eventually pick additional specificly known items for the type
              AbstractType::Ptr type = functionDecl->type<CppFunctionType>()->arguments()[function.matchedArguments];
              if(type) {
                if(CppEnumerationType::Ptr enumeration = TypeUtils::realType(type, m_duContext->topContext()).cast<CppEnumerationType>()) {
                  Declaration* enumDecl = enumeration->declaration(m_duContext->topContext());
                  if(enumDecl && enumDecl->internalContext()) {
                    
                    QualifiedIdentifier prefix = requiredPrefix(enumDecl);
                    
                    DUContext* enumInternal = enumDecl->internalContext();
                    foreach(Declaration* enumerator, enumInternal->localDeclarations()) {
                      QualifiedIdentifier id = prefix + enumerator->identifier();
                      items << CompletionTreeItemPointer(new NormalDeclarationCompletionItem( DeclarationPointer(enumerator), Ptr(this), 0 ));
                      static_cast<NormalDeclarationCompletionItem*>(items.back().data())->alternativeText = id.toString();
                      static_cast<NormalDeclarationCompletionItem*>(items.back().data())->useAlternativeText = true;
                    }
                  }
                }
              }
            }
          }
      }
    }
  }
  
  //Eventually add missing include-completion in cases like NotIncludedClass|
  if(!m_followingText.trimmed().isEmpty()) {
    QString totalExpression = m_followingText.trimmed();
    uint oldItemCount = items.count();
    items += missingIncludeCompletionItems(totalExpression, m_followingText.trimmed() + ": ", ExpressionEvaluationResult(), m_duContext.data(), 0);
    kDebug() << QString("added %1 missing-includes for %2").arg(items.count()-oldItemCount).arg(totalExpression);
  }
}

#endif

}
