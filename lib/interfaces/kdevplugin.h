/***************************************************************************
 *   Copyright (C) 1999-2001 by Bernd Gehrmann                             *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _KDEVPLUGIN_H_
#define _KDEVPLUGIN_H_

#include <qobject.h>
#include <kxmlguiclient.h>

#include <kdevapi.h>

class KDevCore;
class KDevProject;
class KDevVersionControl;
class KDevLanguageSupport;
class KDevEditorManager;
class KDevMakeFrontend;
class KDevAppFrontend;
class KDevPartController;
class KDevMainWindow;
class KDevDebugger;
class KDevDiffFrontend;
class KDevCreateFile;
class ClassStore;

class DCOPClient;
class KAboutData;
class QDomElement;

namespace KParts
{
    class Part;
}

// 2002-02-08 added ccClassStore( ) - daniel

/**
 * This is the base class for all components like doc tree view, LFV, RFV
 * and so on.
 */
class KDevPlugin : public QObject, public KXMLGUIClient
{
    Q_OBJECT

public:
	typedef QMap<QString,KDevVersionControl*> VersionControlMap;
    /**
     * Constructs a component.
     */
    KDevPlugin( const QString& pluginName, const QString& icon, QObject *parent, const char *name=0 );

    /**
     * Destructs a component.
     */
    ~KDevPlugin();

    /**
     * Offers access to KDevelop core.
     **/
    QString pluginName() const;

    /**
     * Returns the name of the icon
     **/
    QString icon() const;

    /**
     * Returns the short description
     **/
    virtual QString shortDescription() const;

    /**
     * Returns the description
     **/
    virtual QString description() const;

    /**
     * Create the DCOP interface for the given @p serviceType, if this
     * plugin provides it. Return false otherwise.
     */
    virtual bool createDCOPInterface( const QString& /*serviceType*/ ) { return 0L; }

    /**
     * Reimplement this method and return a @ref QStringList of all config
     * modules your application part should offer via KDevelop. Note that the
     * part and the module will have to take care for config syncing themselves.
     * Usually @p DCOP used for that purpose.
     *
     * @note Make sure you offer the modules in the form:
     * <code>"pathrelativetosettings/mysettings.desktop"</code>
     *
     **/
    virtual QStringList configModules() const { return QStringList(); };

    /**
     * Reimplement this method if you want to add your credits to the KDevelop
     * about dialog.
     **/
    virtual KAboutData* aboutData() { return 0L; };

    /**
     *  reimplement and retun the part here.You can use this method if
     *  you need to access the current part.
     **/
    virtual KParts::Part* part() { return 0; }

    /**
     * Retrieve the current DCOP Client for the plugin.
     *
     * The clients name is taken from the name argument in the constructor.
     * @note: The DCOPClient object will only be created when this method is
     * called for the first time. Make sure that the part has been loaded
     * before calling this method, if it's the one that contains the DCOP
     * interface that other parts might use.
     */
    DCOPClient *dcopClient() const;

    /**
     * Returns the widget of the plugin. This must be overridden.
     */
    virtual QWidget* widget() { return 0L; }

    /**
     * Gives a reference to the toplevel widget.
     */
    KDevMainWindow *mainWindow();
    /**
     * Indicates whether a valid main window is active.
     */
    bool mainWindowValid();

    /**
     * Gives a reference to the application core
     */
    KDevCore *core();
    /**
     * Gives a reference to the current project component
     */
    KDevProject *project();
    /**
     * Gives a reference to the language support component
     */
    KDevLanguageSupport *languageSupport();
    /**
     * Gives a reference to the make frontend component
     */
    KDevMakeFrontend *makeFrontend();
    /**
     * Gives a reference to the diff frontend component
     */
    KDevDiffFrontend *diffFrontend();
    /**
     * Gives a reference to the application frontend component
     */
    KDevAppFrontend *appFrontend();
    /**
     * Gives a reference to the class store
     */
    ClassStore *classStore();

    /**
     * Gives a reference to the codecompletion classstore
     * added by daniel
     */
    ClassStore* ccClassStore( );

    /**
     * Gives a reference to the DOM tree that represents
     * the project file.
     */
    QDomDocument *projectDom() const;

    /**
     * Returns a reference to the part controller.
     */
    KDevPartController *partController();

    /**
     * Returns a reference to the debugger API.
     */
    KDevDebugger *debugger();

    /**
     * Returns a reference to the Create File API.
     */
    KDevCreateFile *createFileSupport();

    /**
     * To restore any settings which differs from project to project,
     * you can override this base class method to read in from a certain subtree
     * of the project session file.
     * During project loading, respectively project session (.kdevses) loading,
     * this method will be called to give a chance to adapt the part to
     * the newly loaded project. For instance, the debugger part might restore the
     * set breakpoints from the previous debug session for the certain project.
     *
     * Note: Take attention to the difference to common not-project-related session stuff.
     *       They belong to the application rc file (kdeveloprc or gideonrc)
     */
    virtual void restorePartialProjectSession(const QDomElement* el);

    /**
     * See @ restorePartialProjectSession. This is the other way round, the same just for saving.
     */
    virtual void savePartialProjectSession(QDomElement* el);

	/**
	* Add @p vcs to the list of the registered VCS.
	*/
	void registerVersionControl( KDevVersionControl *vcs );

	/**
	* Returns the Version Control System having the specified uinque identifier @p uid.
	*/
	KDevVersionControl *versionControlByName( const QString &uid ) const;

	/**
	* Returns a list with unique identifiers among the version control systems.
	*/
	QStringList registeredVersionControls() const;

	/**
	* Remove (<u>not</u> delete) @p vcs to the list of the registered VCS.
	*/
	void unregisterVersionControl( KDevVersionControl *vcs );

signals:
    /**
     * Emitted when the part will be shown. If you really want to avoid that
     * the part is shown at all, you will have to reimplement showPart();
     **/
    void aboutToShowPart();

protected:
    /**
     * This will cause the part to show up by calling  KPart::show();
     **/
    virtual void showPart();

private:
    KDevApi *m_api;
    class Private;
    Private *d;
};

#endif
