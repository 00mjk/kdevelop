/***************************************************************************
 *   Copyright (C) 2001-2002 by Bernd Gehrmann                             *
 *   bernd@kdevelop.org                                                    *
*                                                                         *
*   Copyright (C) 2002 by Victor R�der                                    *
*   victor_roeder@gmx.de                                                  *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _AUTOPROJECTPART_H_
#define _AUTOPROJECTPART_H_

#include <qdict.h>
#include <qguardedptr.h>
#include <qmap.h>
#include <qdatetime.h>
#include <kdevgenericfactory.h>

#include "kdevproject.h"

class QDomElement;
class QStringList;
class KDialogBase;
class AutoProjectWidget;
class KSelectAction;
class TargetItem;
class ConfigWidgetProxy;


class AutoProjectPart : public KDevProject
{
    Q_OBJECT

public:
    AutoProjectPart( QObject *parent, const char *name, const QStringList &args );
    virtual ~AutoProjectPart();

    /**
     * Implementation of the KDevProject interface.
     */
    virtual QString projectDirectory() const;
    virtual QString projectName() const;
    virtual DomUtil::PairList runEnvironmentVars() const;
    virtual QString runDirectory() const;
    virtual QString mainProgram(bool relative = false) const;
    virtual QString runArguments() const;
    virtual QString activeDirectory() const;
    virtual QStringList allFiles() const;
    virtual void addFile(const QString &fileName);
    virtual void addFiles ( const QStringList& fileList );
    virtual void removeFile(const QString &fileName);
    virtual void removeFiles ( const QStringList& fileList );
    virtual QString buildDirectory() const;
    virtual Options options() const;

	/**
	 * Implementation of the KDevPlugin interface.
	 */
	virtual void restorePartialProjectSession ( const QDomElement* el );
	virtual void savePartialProjectSession ( QDomElement* el );

    /**
     * automake specific methods.
     */
    QStringList allBuildConfigs() const;
    QString currentBuildConfig() const;
    QString topsourceDirectory() const;
    void startMakeCommand(const QString &dir, const QString &target, bool withKdesu = false);
    void buildTarget(QString relpath, TargetItem* titem);

    void needMakefileCvs();
    bool isDirty();
    bool isKDE() const;
    QStringList distFiles() const;

protected:
    /**
     * Reimplemented from KDevProject. These methods are only
     * for use by the application core.
     */
    virtual void openProject(const QString &dirName, const QString &projectName);
    virtual void closeProject();

private slots:
//    void projectConfigWidget(KDialogBase *dlg);
    void slotAddTranslation();
    void slotBuild();
    void slotBuildActiveTarget();
    void slotCompileFile();
    void slotClean();
    void slotDistClean();
    void slotInstall();
    void slotInstallWithKdesu();
    void slotMakefilecvs();
    void slotMakeMessages();
    void slotConfigure();
    void slotExecute();
    void slotExecute2();
    void slotBuildConfigChanged(const QString &config);
    void slotBuildConfigAboutToShow();
    void slotCommandFinished( const QString& command );
    void slotCommandFailed( const QString& command );
    //void slotImportExisting();
	void insertConfigWidget( const KDialogBase* dlg, QWidget * page, unsigned int );

private:
    QGuardedPtr<AutoProjectWidget> m_widget;
    QString m_projectName;
    QString m_projectPath;
    KSelectAction *buildConfigAction;

    QString makeEnvironment() const;
    void setWantautotools();
    QString makefileCvsCommand() const;
    QString configureCommand() const;

    QMap<QString, QDateTime> m_timestamp;
    bool m_executeAfterBuild;
    QString m_buildCommand;
    bool m_needMakefileCvs;
    bool m_lastCompilationFailed;
    bool m_isKDE;

	ConfigWidgetProxy * _configProxy;

    // Enble AutoProjectWidget to emit our signals
    friend class AutoProjectWidget;
    friend class AddSubprojectDialog;
    friend class FileItem;

    // For make commands queueing
    QString constructMakeCommandLine(const QString &dir, const QString &target) const;
    void queueInternalLibDependenciesBuild(TargetItem* titem);
};

typedef KDevGenericFactory<AutoProjectPart> AutoProjectFactory;

#endif
