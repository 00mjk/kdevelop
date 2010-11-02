/***************************************************************************
 *   Copyright 1999-2001 by Bernd Gehrmann                                 *
 *   bernd@kdevelop.org                                                    *
 *   Copyright 2008 by Hamish Rodda                                        *
 *   rodda@kde.org                                                         *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef GREPJOB_H
#define GREPJOB_H

#include <QRegExp>
#include <QDir>
#include <QPointer>

#include <kurl.h>
#include <kjob.h>

#include <interfaces/istatus.h>

#include "grepfindthread.h"
#include "grepoutputmodel.h"

//FIXME: only for benchmarks
#include <QElapsedTimer>

namespace KDevelop
{
    class IProject;
    class ProcessLineMaker;
}

class GrepJob : public KJob, public KDevelop::IStatus
{
    Q_OBJECT
    Q_INTERFACES( KDevelop::IStatus )

public:
    GrepJob( QObject *parent = 0 );

    void setPatternString(const QString& patternString);
    void setTemplateString(const QString &templateString);
    void setReplacementTemplateString(const QString &replTmplString);
    void setReplaceString(const QString &replaceString);
    void setFilesString(const QString &filesString);
    void setExcludeString(const QString &excludeString);
    void setDirectory(const KUrl &directory);
    void setRecursive(bool recursive);
    void setRegexpFlag(bool regexpFlag);
    void setCaseSensitive(bool caseSensitive);
    void setProjectFilesFlag(bool projectFilesFlag);
    void setReplaceFlag(bool replaceFlag);

    virtual void start();

    virtual QString statusName() const;
protected:
    virtual bool doKill();

    GrepOutputModel* model() const;

private Q_SLOTS:
    void slotFindFinished();

Q_SIGNALS:
    void clearMessage( KDevelop::IStatus* );
    void showMessage( KDevelop::IStatus*, const QString & message, int timeout = 0);
    void showErrorMessage(const QString & message, int timeout = 0);
    void hideProgress( KDevelop::IStatus* );
    void showProgress( KDevelop::IStatus*, int minimum, int maximum, int value);

    void foundMatches( const QString& filename, const GrepOutputItem::List& matches);

//FIXME: only for benchmarks, to be deleted
private slots:
    void doBench();
private:
    QElapsedTimer m_timer;

private:
    Q_INVOKABLE void slotWork();

    QString m_patternString;
    QRegExp m_regExp;
    QString m_regExpSimple;
	GrepOutputModel *m_outputModel;
    
    enum {
        WorkCollectFiles,
        WorkGrep,
        WorkReplace,
        WorkIdle
    } m_workState;
    
    KUrl::List m_fileList;
    int m_fileIndex;
    QPointer<GrepFindFilesThread> m_findThread;
    
    KDevelop::DocumentChangeSet m_changeSet;

    QString m_templateString;
    QString m_replacementTemplateString;
    QString m_replaceString;
    QString m_finalReplacement;   // computed with m_replaceString and m_replacementTemplateString
    QString m_filesString;
    QString m_excludeString;
    KUrl m_directory;

    bool m_useProjectFilesFlag;
    bool m_regexpFlag;
    bool m_recursiveFlag;
    bool m_caseSensitiveFlag;
    bool m_replaceFlag;
};

//FIXME: this function is used externally only for tests, find a way to keep it 
//       static for a regular compilation
GrepOutputItem::List grepFile(const QString &filename, const QRegExp &re, const QString &repl);

#endif
