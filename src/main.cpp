#include <config.h>


#include <kaboutdata.h>
#include <kapplication.h>
#include <kcmdlineargs.h>
#include <klocale.h>


#include "toplevel.h"
#include "core.h"
#include "api.h"
#include "plugincontroller.h"
#include "projectmanager.h"


int main(int argc, char *argv[])
{
  static const char *description = I18N_NOOP("Gideon");

  KAboutData aboutData("gideon", I18N_NOOP("Gideon"),
                       VERSION, description, KAboutData::License_GPL,
                       "(c) 1999-2001 Bernd Gehrmann & others");
  aboutData.addAuthor("Bernd Gehrmann", "Everything not credited elsewhere :-)", "bernd@kdevelop.org");
  aboutData.addAuthor("Richard Dale", "Java & Objective C support", "Richard_Dale@tipitina.demon.co.uk");
  aboutData.addAuthor("Jonas Nordin", "C++ source parser", "jonas.nordin@syncom.se");
  aboutData.addAuthor("John Birch", "Debugger frontend", "jbb@kdevelop.org");
  aboutData.addAuthor("Sandy Meier", "PHP support, context menu stuff", "smeier@kdevelop.org");
  aboutData.addAuthor("Kurt Granroth", "KDE application templates", "kurth@granroth.org");
  aboutData.addAuthor("Ian Reinhart Geiser", "Application templates", "geiseri@yahoo.com");
  aboutData.addAuthor("Matthias Hoelzer-Kluepfel", "Several parts, htdig indexing", "hoelzer@kde.org");
  aboutData.addAuthor("The KWrite authors", "editor component", "kwrite-devel@kde.org");
  aboutData.addAuthor("Robert Moniot", "ftnchek documentation", "moniot@fordham.edu");
  aboutData.addAuthor("Ka-Ping Yee", "pydoc utility", "ping@lfw.org");
  aboutData.addAuthor("Dimitri van Heesch", "doxywizard", "dimitri@stack.nl");
  aboutData.addAuthor("Harald Fernengel", "Qt 3 port, patches", "harry@kdevelop.org");
  aboutData.addAuthor("Roberto Raggi", "QEditor part, code completion", "<raggi@cli.di.unipi.it>");
  KCmdLineArgs::init(argc, argv, &aboutData);

  KApplication app;

  // Note: the order is important here!
  API::createInstance();
  TopLevel::createInstance();
  Core::createInstance();
  PluginController::createInstance();
  ProjectManager::createInstance();

  app.setTopWidget(TopLevel::getInstance());
  TopLevel::getInstance()->show();

  TopLevel::getInstance()->loadSettings();

  Core::getInstance()->doEmitCoreInitialized();

  return app.exec();
}
