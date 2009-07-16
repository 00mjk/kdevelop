/*
   Copyright 2009 Ramón Zarazúa <killerfox512+kde@gmail.com>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef MAKEIMPLEMENTATIONPRIVATE_H
#define MAKEIMPLEMENTATIONPRIVATE_H

#include <language/codegen/codegenerator.h>
#include <parsesession.h>

#include <bitset>

namespace KDevelop
{

class ClassMemberDeclaration;
/*!
 * @brief   Applies the PIMPL pattern to a class, hiding all it's private members in a private class
 * @todo    Add support for non-default constructors
 */
class MakeImplementationPrivate : public CodeGenerator<ParseSession>
{
    typedef QList<QMap<IndexedString, QList<SimpleRange> > > UseList;
    
    ///Add policy for declaring in unnamed namespace
    enum Policies
    {
        ContainerIsClass,             //Indicates the container type will be class, otherwise struct will be used
        MoveInitializationToPrivate,  //Moves the initialization of variables to the initialization list of private implementation
        MoveMethodsToPrivate,         //Moves the private methods into the private implementation
        PolicyNum
    };
    
  public:
    MakeImplementationPrivate() : m_classContext(0) {}
    ~MakeImplementationPrivate() {}
    // Implementations from CodeGenerator
    virtual bool process(void);
    virtual bool gatherInformation(void);
    virtual bool checkPreconditions(DUContext* context, const DocumentRange& position);
  
  private:
    DUContext * m_classContext;
    DeclarationPointer m_classDeclaration;
    QString m_privatePointerName;
    QString m_structureName;
    QList<ClassMemberDeclaration *> m_members;
    
    std::bitset<PolicyNum> m_policies;
    
    bool classHasPrivateImplementation();
    void gatherPrivateMembers();
    void updateConstructors(const KDevelop::Declaration &);
    void updateDestructor(void);
    void updateAllUses(UseList & alluses);
};

}

#endif // MAKEIMPLEMENTATIONPRIVATE_H
