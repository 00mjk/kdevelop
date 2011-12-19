/* KDevelop CMake Support
 *
 * Copyright 2007 Aleix Pol <aleixpol@gmail.com>
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

#include "variablemap.h"
#include <QDebug>

VariableMap::VariableMap()
{
    m_scopes.push(QSet<QString>());
}

void VariableMap::insert(const QString& varName, const QStringList& value, bool parentScope)
{
    QSet< QString >* current;
//     qDebug() << "leeeeeeeeeeeeE" << varName << value << parentScope;
    if(parentScope && m_scopes.size()>1) { //TODO: provide error?
        current = &m_scopes[m_scopes.size()-2];
        m_scopes.top().remove(varName);
    } else
        current = &m_scopes.top();
    
    bool inscope=current->contains(varName);
    if(!inscope)
        current->insert(varName);
    
    QStringList ret;
    foreach(const QString& v, value)
    {
        if(v.isEmpty())
            continue;
        
        ret += v.split(';');
    }
    
    if(inscope)
        (*this)[varName]=ret;
    else
        QHash<QString, QStringList>::insertMulti(varName, ret);
    
//     QHash<QString, QStringList>::insert(varName, ret);
//     qDebug() << "++++++++" << varName << QHash<QString, QStringList>::value(varName)/* << *current*/;
}

QStringList VariableMap::value(const QString& varName) const
{
    QStringList ret = QHash<QString, QStringList>::value(varName);
//     qDebug() << "--------" << varName << ret;
    return ret;
}

QHash<QString, QStringList>::iterator VariableMap::insertMulti(const QString & varName, const QStringList & value)
{
    QStringList ret;
    foreach(const QString& v, value)
    {
        if(v.isEmpty())
            continue;
        
        ret += v.split(';');
    }
    
    return QHash<QString, QStringList>::insertMulti(varName, ret);
}

void VariableMap::insertGlobal(const QString& varName, const QStringList& value)
{
    QHash<QString, QStringList>::insert(varName, value);
}

void VariableMap::pushScope()
{
    m_scopes.push(QSet<QString>());
}

void VariableMap::popScope()
{
    QSet<QString> t=m_scopes.pop();
    foreach(const QString& var, t) {
//         qDebug() << "removing........" << var/* << QHash<QString, QStringList>::value(var)*/;
        remove(var);
    }
}
