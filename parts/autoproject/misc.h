/***************************************************************************
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _MISC_H_
#define _MISC_H_

#include <qcstring.h>
#include <qmap.h>
#include <qwidget.h>


/**
 * Very small helper class. It has just one static method which
 */
class AutoProjectTool
{
public:
    /**
     * Loads the compiler options plugin for the given compiler, executes the dialog
     * with some initial flags, and returns the new flags.
     */
    static QString execFlagsDialog(const QString &compiler, const QString &flags, QWidget *parent);
    /**
     * Returns the canonicalized version of a file name, i.e.
     * the file name with special characters replaced by underscores
     */
    static QCString canonicalize(QString str);
    /**
     * 
    /**
     * Parses a Makefile.am and stores its variable assignments
     * in a map.
     */
    static void parseMakefileam(const QString &filename, QMap<QCString,QCString> *variables);
    static void modifyMakefileam(const QString &filename, QMap<QCString,QCString> variables);
};

#endif
