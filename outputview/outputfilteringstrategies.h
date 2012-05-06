/*
    This file is part of KDevelop
    Copyright (C) 2012  Morten Danielsen Volden mvolden2@gmail.com

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#ifndef OUTPUTFILTERINGSTRATEGIES_H
#define OUTPUTFILTERINGSTRATEGIES_H

/**
 * @file This file contains Concrete strategies for filtering output
 * in KDevelop output model. I.e. classes that inherit from ifilterstrategy.
 * New filtering strategies should be added here
 */


#include "ifilterstrategy.h"
#include <outputview/outputviewexport.h>


#include <KUrl>

#include <QLinkedList>

namespace KDevelop
{

/**
* This filter strategy is for not applying any filtering at all. Implementation of the
* interface methods are basically noops
**/
class KDEVPLATFORMOUTPUTVIEW_EXPORT NoFilterStrategy : public IFilterStrategy
{

public:
    NoFilterStrategy();

    virtual bool isErrorInLine(QString const& line, FilteredItem& item);

    virtual bool isActionInLine(QString const& line, FilteredItem& item);

};

/**
 * This filter stategy checks if a given line contains output
 * that is defined as an error (or an action) from a compiler.
 **/
class KDEVPLATFORMOUTPUTVIEW_EXPORT CompilerFilterStrategy : public IFilterStrategy
{

public:
    CompilerFilterStrategy(KUrl const& buildDir);
    virtual ~CompilerFilterStrategy();

    virtual bool isErrorInLine(QString const& line, FilteredItem& item);

    virtual bool isActionInLine(QString const& line, FilteredItem& item);

private:
    KUrl urlForFile( const QString& ) const;

    QLinkedList<QString> m_currentDirs;
    KUrl m_buildDir;

    typedef QMap<QString, QLinkedList<QString>::iterator> PositionMap;
    PositionMap m_positionInCurrentDirs;
};

/**
* This filter stategy filters out errors (no actions) from Python and PHP scripts.
**/
class KDEVPLATFORMOUTPUTVIEW_EXPORT ScriptErrorFilterStrategy : public IFilterStrategy
{

public:
    ScriptErrorFilterStrategy();

    virtual bool isErrorInLine(QString const& line, FilteredItem& item);

    virtual bool isActionInLine(QString const& line, FilteredItem& item);

};

/**
* This filter stategy filters out errors (no actions) from Static code analysis tools (Cppcheck,)
**/
class KDEVPLATFORMOUTPUTVIEW_EXPORT StaticAnalysisFilterStrategy : public IFilterStrategy
{

public:
    StaticAnalysisFilterStrategy();

    virtual bool isErrorInLine(QString const& line, FilteredItem& item);

    virtual bool isActionInLine(QString const& line, FilteredItem& item);

};

} // namespace KDevelop
#endif // OUTPUTFILTERINGSTRATEGIES_H

