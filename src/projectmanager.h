#ifndef __PROJECTMANAGER_H__
#define __PROJECTMANAGER_H__


#include <qstring.h>
#include <qobject.h>


#include <kxmlguiclient.h>
#include <kservice.h>

class KAction;
class KSelectAction;
class KService;
class ProjectInfo;
class ProjectSession;
class KRecentFilesAction;

class ProjectManager : public QObject, public KXMLGUIClient
{
  Q_OBJECT

public:

  ~ProjectManager();

  static QString projectDirectory( const QString& path, bool absolute );

  static ProjectManager *getInstance();

  void loadSettings();
  void saveSettings();
  void loadDefaultProject();

  bool projectLoaded() const;

  KURL projectFile() const;

  void createActions( KActionCollection* ac );

  ProjectSession* projectSession() const;

public slots:
  bool loadProject( const KURL& url);
  bool loadKDevelop2Project( const KURL& url);
  bool closeProject();

private slots:
  void slotOpenProject();
  void slotProjectOptions();

  void loadLocalParts();
  void unloadLocalParts();

  void updateActiveLangMenu();
  void switchLanguage(const QString& lang);

private:
  ProjectManager();

  void setupActions();
  void getGeneralInfo();
  bool checkNewService(const KService::Ptr &service);

  bool loadProjectFile();
  bool saveProjectFile();

  bool loadProjectPart();
  void unloadProjectPart();

  bool loadLanguageSupport(const QString& lang);
  void unloadLanguageSupport();

  bool loadCreateFileSupport();
  void unloadCreateFileSupport();

  ProjectInfo *m_info;

  KAction *m_closeProjectAction, *m_projectOptionsAction;
  KRecentFilesAction *m_openRecentProjectAction;
  KSelectAction *m_activeLanguage;

  static ProjectManager *s_instance;

  ProjectSession* m_pProjectSession;
};


#endif
