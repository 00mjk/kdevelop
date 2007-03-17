/* KDevelop QMake Support
 *
 * Copyright 2006 Andreas Pakulat <apaku@gmx.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#ifndef ASSIGNMENTTEST_H
#define ASSIGNMENTTEST_H

#include <QtCore/QObject>
#include <QtTest/QtTest>
#include <QtCore/QList>

namespace QMake
{
    class StatementAST;
    class ProjectAST;
}

#define TESTASSIGNMENT( ast, var, opval, valcount, joinvalues ) \
    QVERIFY( ast != 0 );\
    QVERIFY( ast->variable() == var );\
    QVERIFY( ast->op() == opval );\
    QVERIFY( ast->values().count() == valcount );\
    QVERIFY( ast->values().join("") == joinvalues );

class AssignmentTest : public QObject
{
        Q_OBJECT
    public:
        AssignmentTest( QObject* parent = 0 );
        ~AssignmentTest();
    private slots:
        void init();
        void cleanup();
        void simpleParsed();
        void simpleParsed_data();
        void assignInValue();
        void assignInValue_data();
        void commentCont();
        void commentCont_data();
    private:
        QMake::ProjectAST* ast;
};

#endif

// kate: space-indent on; indent-width 4; tab-width: 4; replace-tabs on; auto-insert-doxygen on
