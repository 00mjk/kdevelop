/***************************************************************************
 *   Copyright (C) 2003 Alexander Dymo                                     *
 *   cloudtemple@mksat.net                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#ifndef __KDEVPART_PASCALPROJECT_H__
#define __KDEVPART_PASCALPROJECT_H__

#include <qguardedptr.h>

#include "kdevproject.h"

class PascalProjectWidget;
class KDialogBase;
class KDevCompilerOptions;

/**
    Pascal Project: the common project part for all available pascal
    compilers (gpc, fpc, dcc). It holds no project file list and
    tries to abstract from their specifics.
*/
class PascalProjectPart : public KDevProject
{
    Q_OBJECT
public:
    PascalProjectPart(QObject *parent, const char *name, const QStringList &);
    ~PascalProjectPart();

    virtual void openProject(const QString &dirName, const QString &projectName);
    virtual void closeProject();

    /**Returns the name of the main source file without extension.
    All pascal compilers call the binary by that way*/
    virtual QString mainProgram(bool relative = false);
    /**Main source file (like src/main.pp)*/
    virtual QString mainSource();
    virtual void setMainSource(QString fullPath);

    virtual QString projectDirectory();
    virtual QString projectName();
    virtual QString activeDirectory();
    /**The location of the main source file*/
    virtual QString buildDirectory();
    virtual QString runDirectory();
    virtual QString runArguments();
    virtual DomUtil::PairList runEnvironmentVars();

    /**Returns everything in the project directory*/
    virtual QStringList allFiles();
    /**This does absolutelly nothing*/
    virtual void addFile(const QString &fileName);
    /**This does absolutelly nothing*/
    virtual void addFiles(const QStringList &fileList);
    /**This does absolutelly nothing*/
    virtual void removeFile(const QString &fileName);
    /**This does absolutelly nothing*/
    virtual void removeFiles(const QStringList &fileList);

    virtual void changedFiles( const QStringList & fileList );
    virtual void changedFile( const QString & fileName );

    KDevCompilerOptions *createCompilerOptions(const QString &name);

    virtual QString defaultOptions(const QString compiler);

public slots:
    /**loads config from project file*/
    void loadProjectConfig();

private slots:
    void slotBuild();
    void slotExecute();
    void projectConfigWidget(KDialogBase *dlg);
    void configWidget(KDialogBase *dlg);

private:
    QGuardedPtr<PascalProjectWidget> m_widget;

    void listOfFiles(QStringList &result, QString path);

    QString m_buildDir;
    QString m_projectDir;
    QString m_projectName;

    QString m_mainProg;
    QString m_mainSource;
    QString m_compilerExec;
    QString m_compilerOpts;

    QStringList m_sourceFiles;
};

#endif
