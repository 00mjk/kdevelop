/*************************************************************************************
 *  Copyright (C) 2012 by Aleix Pol <aleixpol@kde.org>                               *
 *  Copyright (C) 2012 by Milian Wolff <mail@milianw.de>                             *
 *                                                                                   *
 *  This program is free software; you can redistribute it and/or                    *
 *  modify it under the terms of the GNU General Public License                      *
 *  as published by the Free Software Foundation; either version 2                   *
 *  of the License, or (at your option) any later version.                           *
 *                                                                                   *
 *  This program is distributed in the hope that it will be useful,                  *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of                   *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                    *
 *  GNU General Public License for more details.                                     *
 *                                                                                   *
 *  You should have received a copy of the GNU General Public License                *
 *  along with this program; if not, write to the Free Software                      *
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA   *
 *************************************************************************************/

#ifndef CONTEXTBUILDER_H
#define CONTEXTBUILDER_H

#include <language/duchain/builders/abstractcontextbuilder.h>

#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/qmljsdocument.h>

#include "duchainexport.h"

class ParseSession;

typedef KDevelop::AbstractContextBuilder<QmlJS::AST::Node, QmlJS::AST::IdentifierPropertyName> ContextBuilderBase;

///TODO: cleanup KDevplatform API, remove need for editor integrator
class Editor
{
public:
    Editor(ParseSession** session)
    : m_session(session)
    {}

    ParseSession* parseSession() const
    {
        return *m_session;
    }
private:
    ParseSession** m_session;
};

class KDEVQMLJSDUCHAIN_EXPORT ContextBuilder : public ContextBuilderBase, public QmlJS::AST::Visitor
{
public:
    ContextBuilder();

    virtual void startVisiting(QmlJS::AST::Node* node);
    virtual KDevelop::RangeInRevision editorFindRange(QmlJS::AST::Node* fromNode, QmlJS::AST::Node* toNode);
    virtual KDevelop::QualifiedIdentifier identifierForNode(QmlJS::AST::IdentifierPropertyName* node);

    virtual void setContextOnNode(QmlJS::AST::Node* node, KDevelop::DUContext* context);
    virtual KDevelop::DUContext* contextFromNode(QmlJS::AST::Node* node);

    virtual KDevelop::TopDUContext* newTopContext(const KDevelop::RangeInRevision& range,
                                                  KDevelop::ParsingEnvironmentFile* file = 0);

    void setParseSession(ParseSession* session);

    using Visitor::visit;
    virtual bool visit(QmlJS::AST::FunctionDeclaration* node);

    Editor* editor() const;

    typedef QHash<QmlJS::AST::Node*, KDevelop::DUContext*> NodeToContextHash;
    NodeToContextHash nodeToAstMapping() const;

protected:
    ParseSession* m_session;
    NodeToContextHash m_astToContext;
    bool m_mapAst; // make KDevelop::AbstractContextBuilder happy
    QScopedPointer<Editor> m_editor; // make KDevelop::AbstractUseBuilder happy

};

#endif // CONTEXTBUILDER_H
