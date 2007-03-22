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

#ifndef QMAKEAST_H
#define QMAKEAST_H

#include <QtCore/QString>
#include <QtCore/QList>
#include <QtCore/QStringList>

#include "qmakeexport.h"

//@TODO Port to default constructors and use the set* API to set the various contents; complex constructors are unreadable

namespace QMake
{
    class Driver;

    class QMAKEPARSER_EXPORT AST
    {
        public:
            AST( const QString ws = "", AST* parent = 0 );
            virtual ~AST() = 0;

            AST* parent() const;
            QString whitespace() const;
            void setWhitespace( const QString& );
            virtual void writeToString( QString& ) const = 0 ;
        protected:
            void setParent( AST* );
        private:
            AST* m_parent;
            QString m_ws;
    };


    class QMAKEPARSER_EXPORT StatementAST : public AST
    {
        public:
            StatementAST( const QString& ws = "", AST* parent = 0 ) : AST( ws, parent )
            {}
    };

    class QMAKEPARSER_EXPORT ProjectAST : public AST
    {
        public:
            enum LineEnding {
                Unix = 1,
		MacOS = 2,
		Windows = 4
            };

            ProjectAST( AST* parent = 0 );
            ~ProjectAST();

            QString filename() const;
            void insertStatement( int i, StatementAST* );
            void addStatement( StatementAST* );
            QList<StatementAST*> statements() const;
            void removeStatement( int i );

            void writeToString( QString& ) const;
            void setFilename( const QString& );

            void setLineEnding( LineEnding );
            LineEnding lineEnding();
        private:
            QString m_filename;
            QList<StatementAST*> m_statements;
            LineEnding m_lineEnding;


    };

    class QMAKEPARSER_EXPORT AssignmentAST : public StatementAST
    {
        public:
            AssignmentAST( const QString& variable, const QString& op, const QStringList& values, const QString& = "", const QString& = "", AST* parent = 0 );
            ~AssignmentAST();

            void addValue( const QString& );
            QStringList values() const;
            void removeValue( const QString& );
            QString variable() const;
            void setVariable( const QString& );
            QString op() const;
            void setOp( const QString& );
            void writeToString( QString& ) const;
        private:
            QString m_variable;
            QString m_op;
            QStringList m_values;
            QString m_lineend;
    };

    class QMAKEPARSER_EXPORT NewlineAST : public StatementAST
    {
        public:
            NewlineAST( const QString& ws = "", AST* parent = 0 ) : StatementAST( ws, parent )
            {}
            void writeToString( QString& buf ) const
            {
                buf += whitespace() + "\n";
            }
    };

    class QMAKEPARSER_EXPORT CommentAST : public StatementAST
    {
        public:
            CommentAST( const QString& comment, const QString& ws = "", AST* parent = 0 );
            QString comment() const;
            void setComment( const QString& );
            void writeToString( QString& ) const;
        private:
            QString m_comment;
    };


    class QMAKEPARSER_EXPORT FunctionCallAST : public AST
    {
        public:
            FunctionCallAST( const QString& name, const QString& begin, QStringList args,
                             const QString& end = "", const QString& ws = "", AST* parent = 0 );
            ~FunctionCallAST();
            QStringList arguments() const;
            void insertArgument( int i, const QString& );
            void removeArgument( int i );
            QString functionName() const;
            void setFunctionName( const QString& );
            void writeToString( QString& ) const;
        private:
            QStringList m_args;
            QString m_functionName;
            QString m_begin;
            QString m_end;
    };

    class QMAKEPARSER_EXPORT ScopeBodyAST: public AST
    {
        public:
            ScopeBodyAST( const QString& begin, QList<StatementAST*> stmts,
                          const QString& end = "", AST* parent = 0 );
            ScopeBodyAST( const QString& begin, StatementAST* stmt, AST* parent = 0 );
            ~ScopeBodyAST();
            void insertStatement( int i, StatementAST* );
            void addStatement( StatementAST* );
            QList<StatementAST*> statements() const;
            void removeStatement( int i );
            void writeToString( QString& ) const;
        private:
            QList<StatementAST*> m_statements;
            QString m_begin;
            QString m_end;
    };

    class QMAKEPARSER_EXPORT ScopeAST : public StatementAST
    {
        public:
            enum ScopeType { Function, Simple };
            ScopeAST( FunctionCallAST* call, const QString& lineend = QString(), const QString& ws = "", AST* parent = 0 );
            ScopeAST( const QString& name, const QString& ws = "", AST* parent = 0 );
            ~ScopeAST();
            void writeToString( QString& ) const;
            void setScopeBody( ScopeBodyAST* );
            ScopeBodyAST* scopeBody() const;
            void setScopeName( const QString& );
            QString scopeName( ) const;
            void setFunctionCall( FunctionCallAST* );
            FunctionCallAST* functionCall() const;
        private:
            FunctionCallAST* m_call;
            QString m_scopeName;
            ScopeType m_type;
            ScopeBodyAST* m_body;
            QString m_lineend;
    };

    class QMAKEPARSER_EXPORT OrAST : public StatementAST
    {
        public:
            OrAST( FunctionCallAST* lcall, const QString& orop, FunctionCallAST* rcall,
                   ScopeBodyAST* body,
                   const QString& ws = "", AST* parent = 0 );
            ~OrAST();
            void writeToString( QString& ) const;
            FunctionCallAST* leftCall() const;
            FunctionCallAST* rightCall() const;
            ScopeBodyAST* scopeBody() const;
            void setLeftCall( FunctionCallAST* );
            void setRightCall( FunctionCallAST* );
            void setScopeBody( ScopeBodyAST* );
        private:
            FunctionCallAST* m_lCall;
            FunctionCallAST* m_rCall;
            ScopeBodyAST* m_body;
            QString m_begin;
            QString m_end;
            QString m_orop;
    };

}


#endif

// kate: space-indent on; indent-width 4; tab-width: 4; replace-tabs on; auto-insert-doxygen on
