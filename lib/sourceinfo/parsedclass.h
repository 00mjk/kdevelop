/***************************************************************************
                          parsedstruct.h  -  description
                             -------------------
    begin                : Mon Mar 15 1999
    copyright            : (C) 1999 by Jonas Nordin
    email                : jonas.nordin@syncom.se
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   * 
 *                                                                         *
 ***************************************************************************/

#ifndef _PARSEDCLASS_H_
#define _PARSEDCLASS_H_

#include <qlist.h>
#include <qstringlist.h>
#include <qdict.h>
#include <qstring.h>
#include "parseditem.h"
#include "parsedparent.h"
#include "parsedattribute.h"
#include "parsedmethod.h"
#include "parsedclasscontainer.h"


/**
 * This is the representation of a class that has been parsed by 
 * the classparser.
 * @author Jonas Nordin
 */
class ParsedClass : public ParsedClassContainer
{
public:
    ParsedClass();
    ~ParsedClass();
    
private:
    
    /** List of all slots. */
    QList<ParsedMethod> slotList;
    
    /** All slots ordered by name and argument. */
    QDict<ParsedMethod> slotsByNameAndArg;
    
    /** List of all signals. */
    QList<ParsedMethod> signalList;
    
    /** All signals ordered by name and argument. */
    QDict<ParsedMethod> signalsByNameAndArg;
    
public:
    
    /** List with names of parent classes(if any). */
    QList<ParsedParent> parents;
    
    /** List of slots. */
    QListIterator<ParsedMethod> slotIterator;
    
    /** List of signals. */
    QListIterator<ParsedMethod> signalIterator;

public:
    
    /**
     * Removes all items in the store with references to the file.
     * @param aFile The file to check references to.
     */
    void removeWithReferences(const QString &aFile);
    
    /**
     * Removes a method matching the specification (from either 'methods'
     * or 'slotList').
     * @param aMethod Specification of the method.
     */
    void removeMethod(ParsedMethod *aMethod);
    
    /** Clears all attribute values. */
    void clearDeclaration();
    
    /**
     * Adds a parent.
     * @param aParent A parent of this class.
     */
    void addParent(ParsedParent *aParent);
    
    /**
     * Adds a friend. 
     * @param aName A friendclass of this class.
     */
    void addFriend(const QString &aName)
        { _friends.append(aName); }
    
    /**
     * Adds a signal. 
     * @param aMethod The signal to add.
     */
    void addSignal(ParsedMethod *aMethod);
    
    /**
     * Adds a slot. 
     * @param aMethod The slot to add.
     */
    void addSlot(ParsedMethod *aMethod);
    
    /** List with names of friend classes(if any). */
    QStringList friends() const
        { return _friends; }

    /** 
     * Sets the state if this is a subclass. 
     * @param aState The new state.
     */
    inline void setIsSubClass(bool aState)
        { _isSubClass = aState; }
    bool isSubClass() const
        { return _isSubClass; }

public:
    
    /**
     * Gets a method by comparing with another method. 
     * @param aMethod Method to compare with.
     */
    ParsedMethod *getMethod(ParsedMethod *aMethod);
    
    /**
     * Gets a signal by using its name and arguments. 
     * @param aName Name and arguments of the signal to fetch.
     */
    ParsedMethod *getSignalByNameAndArg(const QString &aName);
    
    /**
     * Gets a slot by using its' name and arguments. 
     * @param aName Name and arguments of the slot to fetch.
     */
    ParsedMethod *getSlotByNameAndArg(const QString &aName);
    
    /** Gets all signals in sorted order. */
    QList<ParsedMethod> *getSortedSignalList();
    
    /** Gets all slots in sorted order. */
    QList<ParsedMethod> *getSortedSlotList();
    
    /** Gets all virtual methods. */
    QList<ParsedMethod> *getVirtualMethodList();
    
    /**
     * Checks if this class has the named parent. 
     * @param aName Name of the parent to check.
     */
    bool hasParent(const QString &aName);
    
    /** Check if the class has any virtual methods. */
    bool hasVirtual();
    
public:

    /** Outputs the class as text on stdout. */
    void out();

    friend QDataStream &operator<<(QDataStream &s, ParsedClass &arg);

private:
    /** List with names of friend classes(if any). */
    QStringList _friends;
    /** Tells if this class is declared inside another class. */
    bool _isSubClass;
};


QDataStream &operator<<(QDataStream &s, ParsedClass &arg);
QDataStream &operator>>(QDataStream &s, ParsedClass &arg);

#endif
