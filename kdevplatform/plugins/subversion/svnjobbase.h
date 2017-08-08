/***************************************************************************
 *   Copyright 2007 Dukju Ahn <dukjuahn@gmail.com>                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef KDEVPLATFORM_PLUGIN_SVNJOBBASE_H
#define KDEVPLATFORM_PLUGIN_SVNJOBBASE_H

#include <vcs/vcsjob.h>

#include "kdevsvncpp/context_listener.hpp"
#include "kdevsvnplugin.h"
#include "debug.h"

#include <ThreadWeaver/Queueing>

extern "C"
{
#include <svn_wc.h>
}

class SvnInternalJobBase;

namespace ThreadWeaver
{
    class Job;
}

class KDevSvnPlugin;

class SvnJobBase : public KDevelop::VcsJob
{
    Q_OBJECT
public:
    explicit SvnJobBase( KDevSvnPlugin*, KDevelop::OutputJob::OutputJobVerbosity verbosity = KDevelop::OutputJob::Verbose );
    ~SvnJobBase() override;
    virtual SvnInternalJobBase* internalJob() const = 0;
    KDevelop::VcsJob::JobStatus status() const override;
    KDevelop::IPlugin* vcsPlugin() const override;

public Q_SLOTS:
    void askForLogin( const QString& );
    void showNotification( const QString&, const QString& );
    void askForCommitMessage();
    void askForSslServerTrust( const QStringList&, const QString&, const QString&,
                               const QString&, const QString&, const QString&,
                               const QString& );
    void askForSslClientCert( const QString& );
    void askForSslClientCertPassword( const QString& );

protected Q_SLOTS:
    void internalJobStarted();
    void internalJobDone();
    void internalJobFailed();

protected:
    void startInternalJob();
    bool doKill() override;
    KDevSvnPlugin* m_part;
private:
    void outputMessage(const QString &message);
    KDevelop::VcsJob::JobStatus m_status;
};

template<typename InternalJobClass>
class SvnJobBaseImpl : public SvnJobBase
{
public:
    explicit SvnJobBaseImpl(KDevSvnPlugin* plugin,
                   KDevelop::OutputJob::OutputJobVerbosity verbosity = KDevelop::OutputJob::Verbose)
        : SvnJobBase(plugin, verbosity)
    {
        m_job = new InternalJobClass(this);
    }

    SvnInternalJobBase* internalJob() const override
    {
        return m_job;
    }

protected:
    InternalJobClass* m_job = nullptr;
};

#endif
