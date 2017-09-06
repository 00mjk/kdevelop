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

#include "abstractdeclarationnavigationcontext.h"

#include <QTextDocument>

#include <KLocalizedString>

#include "../functiondeclaration.h"
#include "../functiondefinition.h"
#include "../classfunctiondeclaration.h"
#include "../namespacealiasdeclaration.h"
#include "../forwarddeclaration.h"
#include "../types/enumeratortype.h"
#include "../types/enumerationtype.h"
#include "../types/functiontype.h"
#include "../duchainutils.h"
#include "../types/pointertype.h"
#include "../types/referencetype.h"
#include "../types/typeutils.h"
#include "../types/typesystem.h"
#include "../persistentsymboltable.h"
#include <debug.h>
#include <interfaces/icore.h>
#include <interfaces/idocumentationcontroller.h>
#include <duchain/types/typealiastype.h>
#include <duchain/types/structuretype.h>
#include <duchain/classdeclaration.h>

namespace KDevelop {

class AbstractDeclarationNavigationContextPrivate
{
public:
  DeclarationPointer m_declaration;
  bool m_fullBackwardSearch = false;
};

AbstractDeclarationNavigationContext::AbstractDeclarationNavigationContext(const DeclarationPointer& decl,
                                                                           const TopDUContextPointer& topContext,
                                                                           AbstractNavigationContext* previousContext)
  : AbstractNavigationContext((topContext ? topContext : TopDUContextPointer(decl ? decl->topContext() : nullptr)), previousContext)
  , d(new AbstractDeclarationNavigationContextPrivate)
{
  d->m_declaration = decl;

  //Jump from definition to declaration if possible
  FunctionDefinition* definition = dynamic_cast<FunctionDefinition*>(d->m_declaration.data());
  if(definition && definition->declaration())
    d->m_declaration = DeclarationPointer(definition->declaration());
}

AbstractDeclarationNavigationContext::~AbstractDeclarationNavigationContext()
{
}

QString AbstractDeclarationNavigationContext::name() const
{
  if(d->m_declaration.data())
    return prettyQualifiedIdentifier(d->m_declaration).toString();
  else
    return declarationName(d->m_declaration);
}

QString AbstractDeclarationNavigationContext::html(bool shorten)
{
  DUChainReadLocker lock(DUChain::lock(), 300);
  if ( !lock.locked() ) {
    return {};
  }

  clear();
  AbstractNavigationContext::html(shorten);

  modifyHtml()  += "<html><body><p>" + fontSizePrefix(shorten);

  addExternalHtml(prefix());

  if(!d->m_declaration.data()) {
    modifyHtml() += i18n("<br /> lost declaration <br />");
    return currentHtml();
  }

  if(auto context = previousContext()) {
    const QString link = createLink(context->name(), context->name(),
                                    NavigationAction(context));
    modifyHtml() += navigationHighlight(i18n("Back to %1<br />", link));
  }

  QExplicitlySharedDataPointer<IDocumentation> doc;

  if( !shorten ) {
    doc = ICore::self()->documentationController()->documentationForDeclaration(d->m_declaration.data());

    const AbstractFunctionDeclaration* function = dynamic_cast<const AbstractFunctionDeclaration*>(d->m_declaration.data());
    if( function ) {
      htmlFunction();
    } else if( d->m_declaration->isTypeAlias() || d->m_declaration->type<EnumeratorType>() || d->m_declaration->kind() == Declaration::Instance ) {
      if( d->m_declaration->isTypeAlias() )
        modifyHtml() += importantHighlight(QStringLiteral("typedef "));

      if(d->m_declaration->type<EnumeratorType>())
        modifyHtml() += i18n("enumerator ");

      AbstractType::Ptr useType = d->m_declaration->abstractType();
      if(d->m_declaration->isTypeAlias()) {
        //Do not show the own name as type of typedefs
        if(useType.cast<TypeAliasType>())
          useType = useType.cast<TypeAliasType>()->type();
      }

      eventuallyMakeTypeLinks( useType );

      modifyHtml() += ' ' + identifierHighlight(declarationName(d->m_declaration).toHtmlEscaped(), d->m_declaration);

      if(auto integralType = d->m_declaration->type<ConstantIntegralType>()) {
        const QString plainValue = integralType->valueAsString();
        if (!plainValue.isEmpty()) {
          modifyHtml() += QStringLiteral(" = ") + plainValue;
        }
      }

      modifyHtml() += QStringLiteral("<br>");
    }else{
      if( d->m_declaration->kind() == Declaration::Type && d->m_declaration->abstractType().cast<StructureType>() ) {
        htmlClass();
      }
      if ( d->m_declaration->kind() == Declaration::Namespace ) {
        modifyHtml() += i18n("namespace %1 ", identifierHighlight(d->m_declaration->qualifiedIdentifier().toString().toHtmlEscaped(), d->m_declaration));
      } else if ( d->m_declaration->kind() == Declaration::NamespaceAlias ) {
        modifyHtml() += identifierHighlight(declarationName(d->m_declaration).toHtmlEscaped(), d->m_declaration);
      }

      if(d->m_declaration->type<EnumerationType>()) {
        EnumerationType::Ptr enumeration = d->m_declaration->type<EnumerationType>();
        modifyHtml() += i18n("enumeration %1 ", identifierHighlight(d->m_declaration->identifier().toString().toHtmlEscaped(), d->m_declaration));
      }

      if(d->m_declaration->isForwardDeclaration()) {
        ForwardDeclaration* forwardDec = static_cast<ForwardDeclaration*>(d->m_declaration.data());
        Declaration* resolved = forwardDec->resolve(topContext().data());
        if(resolved) {
          modifyHtml() += i18n("(resolved forward-declaration: ");
          makeLink(resolved->identifier().toString(), DeclarationPointer(resolved), NavigationAction::NavigateDeclaration );
          modifyHtml() += i18n(") ");
        }else{
          modifyHtml() += i18n("(unresolved forward-declaration) ");
          QualifiedIdentifier id = forwardDec->qualifiedIdentifier();
          const auto& forwardDecFile = forwardDec->topContext()->parsingEnvironmentFile();
          uint count;
          const IndexedDeclaration* decls;
          PersistentSymbolTable::self().declarations(id, count, decls);
          for(uint a = 0; a < count; ++a) {
            auto dec = decls[a].data();
            if (!dec || dec->isForwardDeclaration()) {
                continue;
            }
            const auto& decFile = forwardDec->topContext()->parsingEnvironmentFile();
            if ((static_cast<bool>(decFile) != static_cast<bool>(forwardDecFile)) ||
                (decFile && forwardDecFile && decFile->language() != forwardDecFile->language()))
            {
                // the language of the declarations must match
                continue;
            }
            modifyHtml() += QStringLiteral("<br />");
            makeLink(i18n("possible resolution from"), DeclarationPointer(dec), NavigationAction::NavigateDeclaration);
            modifyHtml() += ' ' + dec->url().str();
          }
        }
      }
      modifyHtml() += QStringLiteral("<br />");
    }
  }else{
    AbstractType::Ptr showType = d->m_declaration->abstractType();
    if(showType && showType.cast<FunctionType>()) {
      showType = showType.cast<FunctionType>()->returnType();
      if(showType)
        modifyHtml() += labelHighlight(i18n("Returns: "));
    }else  if(showType) {
      modifyHtml() += labelHighlight(i18n("Type: "));
    }

    if(showType) {
      eventuallyMakeTypeLinks(showType);
      modifyHtml() += QStringLiteral(" ");
    }
  }

  QualifiedIdentifier identifier = d->m_declaration->qualifiedIdentifier();
  if( identifier.count() > 1 ) {
    if( d->m_declaration->context() && d->m_declaration->context()->owner() )
    {
      Declaration* decl = d->m_declaration->context()->owner();

      FunctionDefinition* definition = dynamic_cast<FunctionDefinition*>(decl);
      if(definition && definition->declaration())
        decl = definition->declaration();

      if(decl->abstractType().cast<EnumerationType>())
        modifyHtml() += labelHighlight(i18n("Enum: "));
      else
        modifyHtml() += labelHighlight(i18n("Container: "));

      makeLink( declarationName(DeclarationPointer(decl)), DeclarationPointer(decl), NavigationAction::NavigateDeclaration );
      modifyHtml() += QStringLiteral(" ");
    } else {
      QualifiedIdentifier parent = identifier;
      parent.pop();
      modifyHtml() += labelHighlight(i18n("Scope: %1 ", typeHighlight(parent.toString().toHtmlEscaped())));
    }
  }

  if( shorten && !d->m_declaration->comment().isEmpty() ) {
    QString comment = QString::fromUtf8(d->m_declaration->comment());
    if( comment.length() > 60 ) {
      comment.truncate(60);
      comment += QLatin1String("...");
    }
    comment.replace('\n', QLatin1String(" "));
    comment.replace(QLatin1String("<br />"), QLatin1String(" "));
    comment.replace(QLatin1String("<br/>"), QLatin1String(" "));
    modifyHtml() += commentHighlight(comment.toHtmlEscaped()) + "   ";
  }


  QString access = stringFromAccess(d->m_declaration);
  if( !access.isEmpty() )
    modifyHtml() += labelHighlight(i18n("Access: %1 ", propertyHighlight(access.toHtmlEscaped())));


  ///@todo Enumerations

  QString detailsHtml;
  QStringList details = declarationDetails(d->m_declaration);
  if( !details.isEmpty() ) {
    bool first = true;
    foreach( const QString &str, details ) {
      if( !first )
        detailsHtml += QLatin1String(", ");
      first = false;
      detailsHtml += propertyHighlight(str);
    }
  }

  QString kind = declarationKind(d->m_declaration);
  if( !kind.isEmpty() ) {
    if( !detailsHtml.isEmpty() )
      modifyHtml() += labelHighlight(i18n("Kind: %1 %2 ", importantHighlight(kind.toHtmlEscaped()), detailsHtml));
    else
      modifyHtml() += labelHighlight(i18n("Kind: %1 ", importantHighlight(kind.toHtmlEscaped())));
  }

  if (d->m_declaration->isDeprecated()) {
    modifyHtml() += labelHighlight(i18n("Status: %1 ", propertyHighlight(i18n("Deprecated"))));
  }

  modifyHtml() += QStringLiteral("<br />");

  if(!shorten)
    htmlAdditionalNavigation();

  if( !shorten ) {
    if(dynamic_cast<FunctionDefinition*>(d->m_declaration.data()))
      modifyHtml() += labelHighlight(i18n( "Def.: " ));
    else
      modifyHtml() += labelHighlight(i18n( "Decl.: " ));

    makeLink( QStringLiteral("%1 :%2").arg( d->m_declaration->url().toUrl().fileName() ).arg( d->m_declaration->rangeInCurrentRevision().start().line()+1 ), d->m_declaration, NavigationAction::JumpToSource );
    modifyHtml() += QStringLiteral(" ");
    //modifyHtml() += "<br />";
    if(!dynamic_cast<FunctionDefinition*>(d->m_declaration.data())) {
      if( FunctionDefinition* definition = FunctionDefinition::definition(d->m_declaration.data()) ) {
        modifyHtml() += labelHighlight(i18n( " Def.: " ));
        makeLink( QStringLiteral("%1 :%2").arg( definition->url().toUrl().fileName() ).arg( definition->rangeInCurrentRevision().start().line()+1 ), DeclarationPointer(definition), NavigationAction::JumpToSource );
      }
    }

    if( FunctionDefinition* definition = dynamic_cast<FunctionDefinition*>(d->m_declaration.data()) ) {
      if(definition->declaration()) {
        modifyHtml() += labelHighlight(i18n( " Decl.: " ));
        makeLink( QStringLiteral("%1 :%2").arg( definition->declaration()->url().toUrl().fileName() ).arg( definition->declaration()->rangeInCurrentRevision().start().line()+1 ), DeclarationPointer(definition->declaration()), NavigationAction::JumpToSource );
      }
    }

    modifyHtml() += QStringLiteral(" "); //The action name _must_ stay "show_uses", since that is also used from outside
    makeLink(i18n("Show uses"), QStringLiteral("show_uses"), NavigationAction(d->m_declaration, NavigationAction::NavigateUses));
  }

  QByteArray declarationComment = d->m_declaration->comment();
  if( !shorten && (!declarationComment.isEmpty() || doc) ) {
    modifyHtml() += QStringLiteral("<p>");

    if(doc) {
      QString comment = doc->description();
      connect(doc.data(), &IDocumentation::descriptionChanged, this, &AbstractDeclarationNavigationContext::contentsChanged);

      if(!comment.isEmpty()) {
        modifyHtml() += "<p>" + commentHighlight(comment) + "</p>";
      }
    }

    QString comment = QString::fromUtf8(declarationComment);
    if(!comment.isEmpty()) {
      // if the first paragraph does not contain a tag, we assume that this is a plain-text comment
      if (!Qt::mightBeRichText(comment)) {
        // still might contain extra html tags for line breaks (this is the case for doxygen-style comments sometimes)
        // let's protect them from being removed completely
        comment.replace(QRegExp("<br */>"), QStringLiteral("\n"));
        comment = comment.toHtmlEscaped();
        comment.replace('\n', QLatin1String("<br />")); //Replicate newlines in html
      }
      modifyHtml() += commentHighlight(comment);
      modifyHtml() += QStringLiteral("</p>");
    }
  }

    if(!shorten && doc) {
      modifyHtml() += "<p>" + i18n("Show documentation for ");
      makeLink(prettyQualifiedName(d->m_declaration),
               d->m_declaration, NavigationAction::ShowDocumentation);
      modifyHtml() += QStringLiteral("</p>");
    }


    //modifyHtml() += "<br />";

  addExternalHtml(suffix());

  modifyHtml() += fontSizeSuffix(shorten) + "</p></body></html>";

  return currentHtml();
}

AbstractType::Ptr AbstractDeclarationNavigationContext::typeToShow(AbstractType::Ptr type) {
  return type;
}

void AbstractDeclarationNavigationContext::htmlFunction()
{
  const AbstractFunctionDeclaration* function = dynamic_cast<const AbstractFunctionDeclaration*>(d->m_declaration.data());
  Q_ASSERT(function);

  const ClassFunctionDeclaration* classFunDecl = dynamic_cast<const ClassFunctionDeclaration*>(d->m_declaration.data());
  const FunctionType::Ptr type = d->m_declaration->abstractType().cast<FunctionType>();
  if( !type ) {
    modifyHtml() += errorHighlight(QStringLiteral("Invalid type<br />"));
    return;
  }

  if( !classFunDecl || (!classFunDecl->isConstructor() && !classFunDecl->isDestructor()) ) {
    // only print return type for global functions and non-ctor/dtor methods
    eventuallyMakeTypeLinks( type->returnType() );
  }

  modifyHtml() += ' ' + identifierHighlight(prettyIdentifier(d->m_declaration).toString().toHtmlEscaped(), d->m_declaration);

  if( type->indexedArgumentsSize() == 0 )
  {
    modifyHtml() += QStringLiteral("()");
  } else {
    modifyHtml() += QStringLiteral("( ");

    bool first = true;
    int firstDefaultParam = type->indexedArgumentsSize() - function->defaultParametersSize();
    int currentArgNum = 0;

    QVector<Declaration*> decls;
    if (DUContext* argumentContext = DUChainUtils::getArgumentContext(d->m_declaration.data())) {
      decls = argumentContext->localDeclarations(topContext().data());
    }
    foreach(const AbstractType::Ptr& argType, type->arguments()) {
      if( !first )
        modifyHtml() += QStringLiteral(", ");
      first = false;

      eventuallyMakeTypeLinks( argType );
      if (currentArgNum < decls.size()) {
        modifyHtml() += ' ' + identifierHighlight(decls[currentArgNum]->identifier().toString().toHtmlEscaped(), d->m_declaration);
      }

      if( currentArgNum >= firstDefaultParam )
        modifyHtml() += " = " + function->defaultParameters()[ currentArgNum - firstDefaultParam ].str().toHtmlEscaped();

      ++currentArgNum;
    }

    modifyHtml() += QStringLiteral(" )");
  }
  modifyHtml() += QStringLiteral("<br />");
}


Identifier AbstractDeclarationNavigationContext::prettyIdentifier(const DeclarationPointer& decl) const
{
  Identifier ret;
  QualifiedIdentifier q = prettyQualifiedIdentifier(decl);
  if(!q.isEmpty())
    ret = q.last();

  return ret;
}

QualifiedIdentifier AbstractDeclarationNavigationContext::prettyQualifiedIdentifier(DeclarationPointer decl) const
{
  if(decl)
    return decl->qualifiedIdentifier();
  else
    return QualifiedIdentifier();
}


QString AbstractDeclarationNavigationContext::prettyQualifiedName(const DeclarationPointer& decl) const
{
  const auto qid = prettyQualifiedIdentifier(decl);
  if (qid.isEmpty()) {
    return i18nc("An anonymous declaration (class, function, etc.)", "<anonymous>");
  }

  return qid.toString();
}

void AbstractDeclarationNavigationContext::htmlAdditionalNavigation()
{
  ///Check if the function overrides or hides another one
  const ClassFunctionDeclaration* classFunDecl = dynamic_cast<const ClassFunctionDeclaration*>(d->m_declaration.data());
  if(classFunDecl) {

    Declaration* overridden = DUChainUtils::getOverridden(d->m_declaration.data());

    if(overridden) {
        modifyHtml() += i18n("Overrides a ");
        makeLink(i18n("function"), QStringLiteral("jump_to_overridden"), NavigationAction(DeclarationPointer(overridden), NavigationAction::NavigateDeclaration));
        modifyHtml() += i18n(" from ");
        makeLink(prettyQualifiedName(DeclarationPointer(overridden->context()->owner())),
                 QStringLiteral("jump_to_overridden_container"),
                 NavigationAction(DeclarationPointer(overridden->context()->owner()),
                                  NavigationAction::NavigateDeclaration));

        modifyHtml() += QStringLiteral("<br />");
    }else{
      //Check if this declarations hides other declarations
      QList<Declaration*> decls;
      foreach(const DUContext::Import &import, d->m_declaration->context()->importedParentContexts())
        if(import.context(topContext().data()))
          decls += import.context(topContext().data())->findDeclarations(QualifiedIdentifier(d->m_declaration->identifier()),
                                                CursorInRevision::invalid(), AbstractType::Ptr(), topContext().data(), DUContext::DontSearchInParent);
      uint num = 0;
      foreach(Declaration* decl, decls) {
        modifyHtml() += i18n("Hides a ");
        makeLink(i18n("function"), QStringLiteral("jump_to_hide_%1").arg(num),
                 NavigationAction(DeclarationPointer(decl),
                                  NavigationAction::NavigateDeclaration));
        modifyHtml() += i18n(" from ");
        makeLink(prettyQualifiedName(DeclarationPointer(decl->context()->owner())),
                 QStringLiteral("jump_to_hide_container_%1").arg(num),
                 NavigationAction(DeclarationPointer(decl->context()->owner()),
                                  NavigationAction::NavigateDeclaration));

        modifyHtml() += QStringLiteral("<br />");
        ++num;
      }
    }

    ///Show all places where this function is overridden
    if(classFunDecl->isVirtual()) {
      Declaration* classDecl = d->m_declaration->context()->owner();
      if(classDecl) {
        uint maxAllowedSteps = d->m_fullBackwardSearch ? (uint)-1 : 10;
        QList<Declaration*> overriders = DUChainUtils::getOverriders(classDecl, classFunDecl, maxAllowedSteps);

        if(!overriders.isEmpty()) {
          modifyHtml() += i18n("Overridden in ");
          bool first = true;
          foreach(Declaration* overrider, overriders) {
            if(!first)
              modifyHtml() += QStringLiteral(", ");
            first = false;

            const auto owner = DeclarationPointer(overrider->context()->owner());
            const QString name = prettyQualifiedName(owner);
            makeLink(name, name, NavigationAction(DeclarationPointer(overrider), NavigationAction::NavigateDeclaration));
          }
          modifyHtml() += QStringLiteral("<br />");
        }
        if(maxAllowedSteps == 0)
          createFullBackwardSearchLink(overriders.isEmpty() ? i18n("Overriders possible, show all") : i18n("More overriders possible, show all"));
      }
    }
  }

  ///Show all classes that inherit this one
  uint maxAllowedSteps = d->m_fullBackwardSearch ? (uint)-1 : 10;
  QList<Declaration*> inheriters = DUChainUtils::getInheriters(d->m_declaration.data(), maxAllowedSteps);

  if(!inheriters.isEmpty()) {
      modifyHtml() += i18n("Inherited by ");
      bool first = true;
      foreach(Declaration* importer, inheriters) {
        if(!first)
          modifyHtml() += QStringLiteral(", ");
        first = false;

        const QString importerName = prettyQualifiedName(DeclarationPointer(importer));
        makeLink(importerName, importerName,
                 NavigationAction(DeclarationPointer(importer), NavigationAction::NavigateDeclaration));
      }
      modifyHtml() += QStringLiteral("<br />");
  }
  if(maxAllowedSteps == 0)
    createFullBackwardSearchLink(inheriters.isEmpty() ? i18n("Inheriters possible, show all") : i18n("More inheriters possible, show all"));
}

void AbstractDeclarationNavigationContext::createFullBackwardSearchLink(const QString& string)
{
  makeLink(string, QStringLiteral("m_fullBackwardSearch=true"), NavigationAction(QStringLiteral("m_fullBackwardSearch=true")));
  modifyHtml() += QStringLiteral("<br />");
}

NavigationContextPointer AbstractDeclarationNavigationContext::executeKeyAction( QString key )
{
  if(key == QLatin1String("m_fullBackwardSearch=true")) {
    d->m_fullBackwardSearch = true;
    clear();
  }
  return NavigationContextPointer(this);
}

void AbstractDeclarationNavigationContext::htmlClass()
{
  StructureType::Ptr klass = d->m_declaration->abstractType().cast<StructureType>();
  Q_ASSERT(klass);

  ClassDeclaration* classDecl = dynamic_cast<ClassDeclaration*>(klass->declaration(topContext().data()));
  if(classDecl) {
    switch ( classDecl->classType() ) {
      case ClassDeclarationData::Class:
        modifyHtml() += QStringLiteral("class ");
        break;
      case ClassDeclarationData::Struct:
        modifyHtml() += QStringLiteral("struct ");
        break;
      case ClassDeclarationData::Union:
        modifyHtml() += QStringLiteral("union ");
        break;
      case ClassDeclarationData::Interface:
        modifyHtml() += QStringLiteral("interface ");
        break;
      case ClassDeclarationData::Trait:
        modifyHtml() += QStringLiteral("trait ");
        break;
      default:
        modifyHtml() += QStringLiteral("<unknown type> ");
        break;
    }
    eventuallyMakeTypeLinks( klass.cast<AbstractType>() );

    FOREACH_FUNCTION( const BaseClassInstance& base, classDecl->baseClasses ) {
      modifyHtml() += ", " + stringFromAccess(base.access) + " " + (base.virtualInheritance ? QStringLiteral("virtual") : QString()) + " ";
      eventuallyMakeTypeLinks(base.baseClass.abstractType());
    }
  } else {
    /// @todo How can we get here? and should this really be a class?
    modifyHtml() += QStringLiteral("class ");
    eventuallyMakeTypeLinks( klass.cast<AbstractType>() );
  }
  modifyHtml() += QStringLiteral(" ");
}

void AbstractDeclarationNavigationContext::htmlIdentifiedType(AbstractType::Ptr type, const IdentifiedType* idType)
{
  Q_ASSERT(type);
  Q_ASSERT(idType);

  if( Declaration* decl = idType->declaration(topContext().data()) ) {

    //Remove the last template-identifiers, because we create those directly
    QualifiedIdentifier id = prettyQualifiedIdentifier(DeclarationPointer(decl));
    Identifier lastId = id.last();
    id.pop();
    lastId.clearTemplateIdentifiers();
    id.push(lastId);

    if(decl->context() && decl->context()->owner()) {

      //Also create full type-links for the context around
      AbstractType::Ptr contextType = decl->context()->owner()->abstractType();
      IdentifiedType* contextIdType = dynamic_cast<IdentifiedType*>(contextType.data());
      if(contextIdType && !contextIdType->equals(idType)) {
        //Create full type information for the context
        if(!id.isEmpty())
          id = id.mid(id.count()-1);
        htmlIdentifiedType(contextType, contextIdType);
        modifyHtml() += QStringLiteral("::").toHtmlEscaped();
      }
    }

    //We leave out the * and & reference and pointer signs, those are added to the end
    makeLink(id.toString() , DeclarationPointer(idType->declaration(topContext().data())), NavigationAction::NavigateDeclaration );
  } else {
    qCDebug(LANGUAGE) << "could not resolve declaration:" << idType->declarationId().isDirect() << idType->qualifiedIdentifier().toString() << "in top-context" << topContext()->url().str();
    modifyHtml() += typeHighlight(type->toString().toHtmlEscaped());
  }
}

void AbstractDeclarationNavigationContext::eventuallyMakeTypeLinks( AbstractType::Ptr type )
{
  type = typeToShow(type);

  if( !type ) {
    modifyHtml() += typeHighlight(QStringLiteral("<no type>").toHtmlEscaped());
    return;
  }

  AbstractType::Ptr target = TypeUtils::targetTypeKeepAliases( type, topContext().data() );
  const IdentifiedType* idType = dynamic_cast<const IdentifiedType*>( target.data() );

  qCDebug(LANGUAGE) << "making type-links for" << type->toString();

  if( idType && idType->declaration(topContext().data()) ) {
    ///@todo This is C++ specific, move into subclass

    if(target->modifiers() & AbstractType::ConstModifier)
      modifyHtml() += typeHighlight(QStringLiteral("const "));

    htmlIdentifiedType(target, idType);

    //We need to exchange the target type, else template-parameters may confuse this
    SimpleTypeExchanger exchangeTarget(target, AbstractType::Ptr());

    AbstractType::Ptr exchanged = exchangeTarget.exchange(type);

    if(exchanged) {
      QString typeSuffixString = exchanged->toString();
      QRegExp suffixExp("\\&|\\*");
      int suffixPos = typeSuffixString.indexOf(suffixExp);
      if(suffixPos != -1)
        modifyHtml() += typeHighlight(typeSuffixString.mid(suffixPos));
    }

  } else {
    if(idType) {
      qCDebug(LANGUAGE) << "identified type could not be resolved:" << idType->qualifiedIdentifier() << idType->declarationId().isValid() << idType->declarationId().isDirect();
    }
    modifyHtml() += typeHighlight(type->toString().toHtmlEscaped());
  }
}

DeclarationPointer AbstractDeclarationNavigationContext::declaration() const
{
  return d->m_declaration;
}

QString AbstractDeclarationNavigationContext::identifierHighlight(const QString& identifier, const DeclarationPointer& decl) const
{
  QString ret = nameHighlight(identifier);
  if (!decl) {
    return ret;
  }

  if (decl->isDeprecated()) {
    ret = QStringLiteral("<i>") + ret + QStringLiteral("</i>");
  }
  return ret;
}

QString AbstractDeclarationNavigationContext::stringFromAccess(Declaration::AccessPolicy access)
{
  switch(access) {
    case Declaration::Private:
      return QStringLiteral("private");
    case Declaration::Protected:
      return QStringLiteral("protected");
    case Declaration::Public:
      return QStringLiteral("public");
    default:
      break;
  }
  return QString();
}

QString AbstractDeclarationNavigationContext::stringFromAccess(const DeclarationPointer& decl)
{
  const ClassMemberDeclaration* memberDecl = dynamic_cast<const ClassMemberDeclaration*>(decl.data());
  if( memberDecl ) {
    return stringFromAccess(memberDecl->accessPolicy());
  }
  return QString();
}

QString AbstractDeclarationNavigationContext::declarationName( const DeclarationPointer& decl ) const
{
  if( NamespaceAliasDeclaration* alias = dynamic_cast<NamespaceAliasDeclaration*>(decl.data()) ) {
    if( alias->identifier().isEmpty() )
      return "using namespace " + alias->importIdentifier().toString();
    else
      return "namespace " + alias->identifier().toString() + " = " + alias->importIdentifier().toString();
  }

  if( !decl )
    return i18nc("A declaration that is unknown", "Unknown");
  else
    return prettyIdentifier(decl).toString();
}

QStringList AbstractDeclarationNavigationContext::declarationDetails(const DeclarationPointer& decl)
{
  QStringList details;
  const AbstractFunctionDeclaration* function = dynamic_cast<const AbstractFunctionDeclaration*>(decl.data());
  const ClassMemberDeclaration* memberDecl = dynamic_cast<const ClassMemberDeclaration*>(decl.data());
  if( memberDecl ) {
    if( memberDecl->isMutable() )
      details << QStringLiteral("mutable");
    if( memberDecl->isRegister() )
      details << QStringLiteral("register");
    if( memberDecl->isStatic() )
      details << QStringLiteral("static");
    if( memberDecl->isAuto() )
      details << QStringLiteral("auto");
    if( memberDecl->isExtern() )
      details << QStringLiteral("extern");
    if( memberDecl->isFriend() )
      details << QStringLiteral("friend");
  }

  if( decl->isDefinition() )
    details << i18nc("tells if a declaration is defining the variable's value", "definition");
  if( decl->isExplicitlyDeleted() )
    details << QStringLiteral("deleted");

  if( memberDecl && memberDecl->isForwardDeclaration() )
    details << i18nc("as in c++ forward declaration", "forward");

  AbstractType::Ptr t(decl->abstractType());
  if( t ) {
    if( t->modifiers() & AbstractType::ConstModifier )
      details << i18nc("a variable that won't change, const", "constant");
    if( t->modifiers() & AbstractType::VolatileModifier )
      details << QStringLiteral("volatile");
  }
  if( function ) {
    if( function->isInline() )
      details << QStringLiteral("inline");
    if( function->isExplicit() )
      details << QStringLiteral("explicit");
    if( function->isVirtual() )
      details << QStringLiteral("virtual");

    const ClassFunctionDeclaration* classFunDecl = dynamic_cast<const ClassFunctionDeclaration*>(decl.data());
    if( classFunDecl ) {
      if( classFunDecl->isSignal() )
        details << QStringLiteral("signal");
      if( classFunDecl->isSlot() )
        details << QStringLiteral("slot");
      if( classFunDecl->isFinal() )
        details << QStringLiteral("final");
      if( classFunDecl->isConstructor() )
        details << QStringLiteral("constructor");
      if( classFunDecl->isDestructor() )
        details << QStringLiteral("destructor");
      if( classFunDecl->isConversionFunction() )
        details << QStringLiteral("conversion-function");
      if( classFunDecl->isAbstract() )
        details << QStringLiteral("abstract");
    }
  }

  return details;
}

}
