/* This file is part of KDevelop
Copyright 2007 Hamish Rodda <rodda@kde.org>

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public License
along with this library; see the file COPYING.LIB.  If not, write to
the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
Boston, MA 02110-1301, USA.
*/

#ifndef IRUN_H
#define IRUN_H

#include <QtCore/QObject>
#include <QtCore/QSharedDataPointer>

#include <KUrl>

#include "interfacesexport.h"
class KJob;

namespace KDevelop
{

/**
 * This class holds all properties which specify a run session.
 */
class KDEVPLATFORMINTERFACES_EXPORT IRun
{
public:
    IRun();
    IRun(const IRun& rhs);
    IRun& operator=(const IRun& rhs);
    virtual ~IRun();

    /**
     * The executable for this session.
     */
    KUrl executable() const;

    /**
     * Set the \a executable to be run.
     */
    void setExecutable(const QString& executable);

    /**
     * The working directory for this session.
     */
    KUrl workingDirectory() const;

    /**
     * Set the \a workingDirectory for this run session.
     */
    void setWorkingDirectory(const QString& workingDirectory);

    /**
     * The key which references the environment under which to run the executable.
     */
    QString environmentKey() const;

    /**
     * Set the environment under which the executable should be run.
     */
    void setEnvironmentKey(const QString& environmentKey);

    /**
     * The argument list to pass to the executable.
     */
    QStringList arguments() const;

    /**
     * Add an argument to pass to the executable.
     */
    void addArgument(const QString& argument);

    /**
     * Set the arguments which should be passed to the executable.
     */
    void setArguments(const QStringList& arguments);

    /**
     * Clear all arguments.
     */
    void clearArguments();

    /**
     * The requested instrumentor, usually one of 'default', 'gdb', 'memcheck' etc.
     */
    QString instrumentor() const;

    /**
     * Set which instrumentor should be used to run the executable.
     */
    void setInstrumentor(const QString& instrumentor);

    /**
     * The argument list to pass to the executable.
     */
    QStringList instrumentorArguments() const;

    /**
     * Add an argument to pass to the executable.
     */
    void addInstrumentorArgument(const QString& argument);

    /**
     * Set the arguments which should be passed to the executable.
     */
    void setInstrumentorArguments(const QStringList& arguments);

    /**
     * Clear all arguments.
     */
    void clearInstrumentorArguments();

    /**
     * Set the jobs that have to be started before running.
     */
    void setDependencies(const QList<KJob*>& c);

    /**
     * Clear all run dependencies.
     */
    void clearDependencies();

    /**
     * The requested run dependencies.
     */
    QList<KJob*> dependencies() const;

    /**
     * Returns the user name or id under whose credentials the program should be run.
     */
    QString runAsUser() const;

    /**
     * Set the user name or id under whose credentials the program should be run.
     */
    void setRunAsUser(const QString& user);

private:
    class IRunPrivate;
    QSharedDataPointer<IRunPrivate> d;
};

}

#endif
