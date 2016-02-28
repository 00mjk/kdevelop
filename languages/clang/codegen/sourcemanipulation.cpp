/*
 * This file is part of KDevelop
 *
 * Copyright 2009 David Nolden <david.nolden.kdevelop@art-master.de>
 * Copyright 2015 Sergey Kalinichev <kalinichev.so.0@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License or (at your option) version 3 or any later version
 * accepted by the membership of KDE e.V. (or its successor approved
 * by the membership of KDE e.V.), which shall act as a proxy
 * defined in Section 14 of version 3 of the license.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "sourcemanipulation.h"
#include <language/codegen/coderepresentation.h>
#include <language/duchain/parsingenvironment.h>
#include <language/duchain/stringhelpers.h>

#include <language/duchain/abstractfunctiondeclaration.h>
#include <language/duchain/classdeclaration.h>
#include <language/duchain/classmemberdeclaration.h>
#include <language/duchain/types/enumeratortype.h>

#include "codegenhelper.h"
#include "util/clangdebug.h"

using namespace KDevelop;

namespace
{
QualifiedIdentifier stripPrefixes(const DUContextPointer& ctx, const QualifiedIdentifier& id)
{
    if (!ctx) {
        return id;
    }

    auto imports = ctx->fullyApplyAliases({}, ctx->topContext());
    if (imports.contains(id)) {
        return {}; /// The id is a namespace that is imported into the current context
    }

    auto basicDecls = ctx->findDeclarations(id, CursorInRevision::invalid(), {}, nullptr,
                                            (DUContext::SearchFlags)(DUContext::NoSelfLookUp | DUContext::NoFiltering));

    if (basicDecls.isEmpty()) {
        return id;
    }

    auto newId = id.mid(1);
    auto result = id;
    while (!newId.isEmpty()) {
        auto foundDecls
            = ctx->findDeclarations(newId, CursorInRevision::invalid(), {}, nullptr,
                                    (DUContext::SearchFlags)(DUContext::NoSelfLookUp | DUContext::NoFiltering));

        if (foundDecls == basicDecls) {
            result = newId; // must continue to find the shortest possible identifier
            // esp. for cases where nested namespaces are used (e.g. using namespace a::b::c;)
            newId = newId.mid(1);
        }
    }

    return result;
}

QString makeSignatureString(const QVector<SourceCodeInsertion::SignatureItem>& signature, const DUContextPointer& context)
{
    QString ret;
    foreach (const auto& item, signature) {
        if (!ret.isEmpty()) {
            ret += QStringLiteral(", ");
        }

        ret += CodegenHelper::simplifiedTypeString(item.type, context.data());

        if (!item.name.isEmpty()) {
            ret += QStringLiteral(" ") + item.name;
        }
    }
    return ret;
}

// Re-indents the code so the leftmost line starts at zero
QString zeroIndentation(const QString& str, int fromLine = 0)
{
    QStringList lines = str.split(QLatin1Char('\n'));
    QStringList ret;

    if (fromLine < lines.size()) {
        ret = lines.mid(0, fromLine);
        lines = lines.mid(fromLine);
    }

    QRegExp nonWhiteSpace(QStringLiteral("\\S"));
    int minLineStart = 10000;
    foreach (const auto& line, lines) {
        int lineStart = line.indexOf(nonWhiteSpace);
        if (lineStart < minLineStart) {
            minLineStart = lineStart;
        }
    }

    foreach (const auto& line, lines) {
        ret << line.mid(minLineStart);
    }

    return ret.join(QStringLiteral("\n"));
}
}

DocumentChangeSet SourceCodeInsertion::changes()
{
    return m_changeSet;
}

void SourceCodeInsertion::setSubScope(const QualifiedIdentifier& scope)
{
    m_scope = scope;

    if (!m_context) {
        return;
    }

    QStringList needNamespace = m_scope.toStringList();

    bool foundChild = true;
    while (!needNamespace.isEmpty() && foundChild) {
        foundChild = false;

        foreach (DUContext* child, m_context->childContexts()) {
            clangDebug() << "checking child" << child->localScopeIdentifier().toString() << "against"
                     << needNamespace.first();
            if (child->localScopeIdentifier().toString() == needNamespace.first() && child->type() == DUContext::Namespace) {
                clangDebug() << "taking";
                m_context = child;
                foundChild = true;
                needNamespace.pop_front();
                break;
            }
        }
    }

    m_scope = stripPrefixes(m_context, QualifiedIdentifier(needNamespace.join(QStringLiteral("::"))));
}

QString SourceCodeInsertion::applySubScope(const QString& decl) const
{
    if (m_scope.isEmpty()) {
        return decl;
    }

    QString scopeType = QStringLiteral("namespace");
    QString scopeClose;

    if (m_context && m_context->type() == DUContext::Class) {
        scopeType = QStringLiteral("struct");
        scopeClose = QStringLiteral(";");
    }

    QString ret;
    foreach (const QString& scope, m_scope.toStringList()) {
        ret += scopeType + QStringLiteral(" ") + scope + QStringLiteral(" {\n");
    }

    ret += decl;
    ret += QStringLiteral("}") + scopeClose + QStringLiteral("\n").repeated(m_scope.count());

    return ret;
}

SourceCodeInsertion::SourceCodeInsertion(TopDUContext* topContext)
    : m_context(topContext)
    , m_topContext(topContext)
    , m_codeRepresentation(createCodeRepresentation(m_topContext->url()))
{
}

SourceCodeInsertion::~SourceCodeInsertion()
{
}

KTextEditor::Cursor SourceCodeInsertion::end() const
{
    auto ret = m_context->rangeInCurrentRevision().end();
    if (m_codeRepresentation && m_codeRepresentation->lines() && dynamic_cast<TopDUContext*>(m_context.data())) {
        ret.setLine(m_codeRepresentation->lines() - 1);
        ret.setColumn(m_codeRepresentation->line(ret.line()).size());
    }
    return ret;
}

QString SourceCodeInsertion::indentation() const
{
    if (!m_codeRepresentation || !m_context || m_context->localDeclarations().isEmpty()) {
        clangDebug() << "cannot do indentation";
        return QString();
    }

    foreach (Declaration* decl, m_context->localDeclarations()) {
        if (decl->range().isEmpty() || decl->range().start.column == 0) {
            continue; // Skip declarations with empty range, that were expanded from macros
        }
        int spaces = 0;

        QString textLine = m_codeRepresentation->line(decl->range().start.line);

        for (int a = 0; a < textLine.size(); ++a) {
            if (textLine.at(a).isSpace()) {
                ++spaces;
            } else {
                break;
            }
        }

        return textLine.left(spaces);
    }

    return {};
}

QString SourceCodeInsertion::applyIndentation(const QString& decl) const
{
    QStringList lines = decl.split(QLatin1Char('\n'));
    QString ind = indentation();
    QStringList ret;
    foreach (const QString& line, lines) {
        if (!line.isEmpty()) {
            ret << ind + line;
        } else {
            ret << line;
        }
    }
    return ret.join(QStringLiteral("\n"));
}

KTextEditor::Range SourceCodeInsertion::insertionRange(int line)
{
    if (line == 0 || !m_codeRepresentation) {
        return KTextEditor::Range(line, 0, line, 0);
    }

    KTextEditor::Range range(line - 1, m_codeRepresentation->line(line - 1).size(), line - 1,
                             m_codeRepresentation->line(line - 1).size());
    // If the context finishes on that line, then this will need adjusting
    if (!m_context->rangeInCurrentRevision().contains(range)) {
        range.start() = m_context->rangeInCurrentRevision().end();
        if (range.start().column() > 0) {
            range.start() = range.start() - KTextEditor::Cursor(0, 1);
        }
        range.end() = range.start();
    }

    return range;
}

bool SourceCodeInsertion::insertFunctionDeclaration(const Identifier& name, const AbstractType::Ptr& _returnType,
                                                    const QVector<SignatureItem>& signature, bool isConstant,
                                                    const QString& body)
{
    if (!m_context) {
        return false;
    }

    auto returnType = _returnType;

    QString decl
        = (returnType ? (CodegenHelper::simplifiedTypeString(returnType, m_context.data()) + QStringLiteral(" ")) : QString())
        + name.toString() + QStringLiteral("(") + makeSignatureString(signature, m_context) + QStringLiteral(")");

    if (isConstant) {
        decl += QStringLiteral(" const");
    }

    if (body.isEmpty()) {
        decl += QStringLiteral(";");
    } else {
        if (!body.startsWith(QLatin1Char(' ')) && !body.startsWith(QLatin1Char('\n'))) {
            decl += QStringLiteral(" ");
        }
        decl += zeroIndentation(body);
    }

    int line = findInsertionPoint();

    decl = QStringLiteral("\n\n") + applyIndentation(applySubScope(decl));

    return m_changeSet.addChange(DocumentChange(m_context->url(), insertionRange(line), QString(), decl));
}

int SourceCodeInsertion::findInsertionPoint() const
{
    int line = end().line();

    foreach (auto decl, m_context->localDeclarations()) {
        if (m_context->type() == DUContext::Class) {
            continue;
        }

        if (!dynamic_cast<AbstractFunctionDeclaration*>(decl)) {
            continue;
        }

        line = decl->range().end.line + 1;
        if (decl->internalContext()) {
            line = decl->internalContext()->range().end.line + 1;
        }
    }

    clangDebug() << line << m_context->scopeIdentifier(true) << m_context->rangeInCurrentRevision()
             << m_context->url().toUrl() << m_context->parentContext();
    clangDebug() << "count of declarations:" << m_context->topContext()->localDeclarations().size();

    return line;
}
