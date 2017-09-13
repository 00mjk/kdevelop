/*
 *    This file is part of KDevelop
 *
 *    Copyright 2013 Olivier de Gaalon <olivier.jg@gmail.com>
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Library General Public
 *    License as published by the Free Software Foundation; either
 *    version 2 of the License, or (at your option) any later version.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Library General Public License for more details.
 *
 *    You should have received a copy of the GNU Library General Public License
 *    along with this library; see the file COPYING.LIB.  If not, write to
 *    the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *    Boston, MA 02110-1301, USA.
 */

#ifndef CLANGINDEX_H
#define CLANGINDEX_H

#include "clanghelpers.h"

#include "clangprivateexport.h"
#include <serialization/indexedstring.h>

#include <util/path.h>

#include <QReadWriteLock>
#include <QSharedPointer>

#include <clang-c/Index.h>

class ClangParsingEnvironment;
class ClangPCH;

class KDEVCLANGPRIVATE_EXPORT ClangIndex
{
public:
    ClangIndex();
    ~ClangIndex();

    CXIndex index() const;

    /**
     * @returns the existing ClangPCH for @p environment
     *
     * The PCH is created using @p environment if it doesn't exist
     * This function is thread safe.
     */
    QSharedPointer<const ClangPCH> pch(const ClangParsingEnvironment& environment);

    /**
     * Gets the currently pinned TU for @p url
     *
     * If the currently pinned TU does not import @p url, @p url is returned
     */
    KDevelop::IndexedString translationUnitForUrl(const KDevelop::IndexedString& url);

    /**
     * Pin @p tu as the translation unit to use when parsing @p url
     */
    void pinTranslationUnitForUrl(const KDevelop::IndexedString& tu, const KDevelop::IndexedString& url);

    /**
     * Unpin any translation unit currently pinned for @p url
     */
    void unpinTranslationUnitForUrl(const KDevelop::IndexedString& url);

private:
    CXIndex m_index;

    QReadWriteLock m_pchLock;
    QHash<KDevelop::Path, QSharedPointer<const ClangPCH>> m_pch;

    QMutex m_mappingMutex;
    QHash<KDevelop::IndexedString, KDevelop::IndexedString> m_tuForUrl;
};

#endif //CLANGINDEX_H
