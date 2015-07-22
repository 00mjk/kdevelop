/* This file is part of KDevelop
 * Copyright 2013 Christoph Thielecke <crissi99@gmx.de>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef _CPPCHECKPARSER_H_
#define _CPPCHECKPARSER_H_

#include <QXmlStreamReader>
#include <QApplication>
#include <QStack>

#include "iparser.h"

namespace cppcheck
{

/**
 * A class which parses cppcheck's XML output
 * and emits signals when items are parsed
 */
class CppcheckParser : public Parser
{
    Q_OBJECT

public:

    CppcheckParser(QObject* parent = 0);
    ~CppcheckParser();

    QVector<KDevelop::IProblem::Ptr> problems() const override{ return m_problems; }

signals:
    void reset();

public slots:

    void parse();


private:
    void storeError();

    // XML parsing
    bool endElement();
    bool startElement();
    void clear();

    enum State {
        Unknown,
        Results,
        CppCheck,
        Errors,
        Error,
        Location
    };

    QStack<State> m_stateStack;
    QString m_buffer;
    QHash<QString, QString> cppcheckArgs, programArgs;

    // error info
    int ErrorLine;
    QString ErrorFile;
    QString Message;
    QString MessageVerbose;
    QString Severity;
    QString ProjectPath;

    QVector<KDevelop::IProblem::Ptr> m_problems;

};
}


#endif /* _CPPCHECKPARSER_H_ */
