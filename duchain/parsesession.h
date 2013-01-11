/*************************************************************************************
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

#ifndef PARSESESSION_H
#define PARSESESSION_H

#include <qmljs/qmljsdocument.h>

#include <language/duchain/indexedstring.h>
#include <language/interfaces/iproblem.h>

#include "duchainexport.h"

namespace KDevelop
{
class SimpleRange;
class RangeInRevision;
}

/**
 * This class wraps the qmljs parser and offers some helper functions
 * that make it simpler to use the parser in KDevelop.
 */
class KDEVQMLJSDUCHAIN_EXPORT ParseSession
{
public:
    /**
     * Convert @p location to a KDevelop::RangeInRevision and return that.
     */
    static KDevelop::RangeInRevision locationToRange(const QmlJS::AST::SourceLocation& location);

    /**
     * Convert @p locationFrom and @p locationTo to a KDevelop::RangeInRevision and return that.
     */
    static KDevelop::RangeInRevision locationsToRange(const QmlJS::AST::SourceLocation& locationFrom,
                                                      const QmlJS::AST::SourceLocation& locationTo);


    /**
     * @return a range that spans @p fromNode and @p toNode.
     */
    static KDevelop::RangeInRevision editorFindRange(QmlJS::AST::Node* fromNode,
                                                     QmlJS::AST::Node* toNode);

    /**
     * @return a unique identifier for QML/JS documents.
     */
    static KDevelop::IndexedString languageString();

    /**
     * Parse the given @p contents.
     *
     * @param url The url for the document you want to parse.
     * @param contents The contents of the document you want to parse.
     */
    ParseSession(const KDevelop::IndexedString& url, const QString& contents);

    /**
     * @return the URL of this session
     */
    KDevelop::IndexedString url() const;

    /**
     * @return true if the document was properly parsed, false otherwise.
     */
    bool isParsedCorrectly() const;

    /**
     * @return the root AST node or null if it failed to parse.
     */
    QmlJS::AST::Node* ast() const;

    /**
     * @return the problems encountered during parsing.
     */
    QList<KDevelop::ProblemPointer> problems() const;

    /**
     * @return the string representation of @p location.
     */
    QString symbolAt(const QmlJS::AST::SourceLocation& location) const;

    /**
     * @return the language of the parsed document.
     */
    QmlJS::Document::Language language() const;

private:
    KDevelop::IndexedString m_url;
    QmlJS::Document::MutablePtr m_doc;
};

#endif // PARSESESSION_H
