/*
    This file is part of KDevelop
    Copyright (C) 2012  Morten Danielsen Volden mvolden2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef KDEVPLATFORM_OUTPUTFILTERINGSTRATEGIES_H
#define KDEVPLATFORM_OUTPUTFILTERINGSTRATEGIES_H

/**
 * @file This file contains Concrete strategies for filtering output
 * in KDevelop output model. I.e. classes that inherit from ifilterstrategy.
 * New filtering strategies should be added here
 */


#include "ifilterstrategy.h"
#include "outputformats.h"

#include <util/path.h>

#include <outputview/outputviewexport.h>

#define KDEVPLATFORMOUTPUTVIEW_TEST_EXPORT KDEVPLATFORMOUTPUTVIEW_EXPORT

#include <QList>
#include <QVector>
#include <QtCore/QMap>
#include <QUrl>

namespace KDevelop
{

/**
 * This filter strategy is for not applying any filtering at all. Implementation of the
 * interface methods are basically noops
 **/
class KDEVPLATFORMOUTPUTVIEW_TEST_EXPORT NoFilterStrategy : public IFilterStrategy
{

public:
    NoFilterStrategy();

    virtual FilteredItem errorInLine(const QString& line) override;

    virtual FilteredItem actionInLine(const QString& line) override;

};

/**
 * This filter stategy checks if a given line contains output
 * that is defined as an error (or an action) from a compiler.
 **/
class KDEVPLATFORMOUTPUTVIEW_TEST_EXPORT CompilerFilterStrategy : public IFilterStrategy
{

public:
    CompilerFilterStrategy(const QUrl& buildDir);

    virtual FilteredItem errorInLine(const QString& line) override;

    virtual FilteredItem actionInLine(const QString& line) override;

    QVector<QString> getCurrentDirs();

private:
    KDevelop::Path pathForFile( const QString& ) const;
    bool isMultiLineCase(ErrorFormat curErrFilter) const;
    void putDirAtEnd(const KDevelop::Path& pathToInsert);

    QVector<KDevelop::Path> m_currentDirs;
    KDevelop::Path m_buildDir;

    using PositionMap = QHash<KDevelop::Path, int>;
    PositionMap m_positionInCurrentDirs;
};

/**
 * This filter stategy filters out errors (no actions) from Python and PHP scripts.
 **/
class KDEVPLATFORMOUTPUTVIEW_TEST_EXPORT ScriptErrorFilterStrategy : public IFilterStrategy
{

public:
    ScriptErrorFilterStrategy();

    virtual FilteredItem errorInLine(const QString& line) override;

    virtual FilteredItem actionInLine(const QString& line) override;

};

/**
 * This filter strategy filters out errors (no actions) from runtime debug output of native applications
 *
 * This is especially useful for runtime output of Qt applications, for example lines such as:
 * "ASSERT: "errors().isEmpty()" in file /tmp/foo/bar.cpp", line 49"
 */
class KDEVPLATFORMOUTPUTVIEW_TEST_EXPORT NativeAppErrorFilterStrategy : public IFilterStrategy
{
public:
    NativeAppErrorFilterStrategy();

    virtual FilteredItem errorInLine(const QString& line) override;
    virtual FilteredItem actionInLine(const QString& line) override;
};

/**
 * This filter stategy filters out errors (no actions) from Static code analysis tools (Cppcheck,)
 **/
class KDEVPLATFORMOUTPUTVIEW_TEST_EXPORT StaticAnalysisFilterStrategy : public IFilterStrategy
{

public:
    StaticAnalysisFilterStrategy();

    virtual FilteredItem errorInLine(const QString& line) override;

    virtual FilteredItem actionInLine(const QString& line) override;

};

} // namespace KDevelop
#endif // KDEVPLATFORM_OUTPUTFILTERINGSTRATEGIES_H

