/*
 * Copyright (C) $YEAR$ $AUTHOR$ <$EMAIL$>
 */

#include "$APPNAMELC$.h"
#include <kapplication.h>
#include <kaboutdata.h>
#include <kcmdlineargs.h>
#include <klocale.h>

static const char description[] =
    I18N_NOOP("A KDE KPart Application");

static const char version[] = "$VERSION$";

static KCmdLineOptions options[] =
{
//    { "+[URL]", I18N_NOOP( "Document to open." ), 0 },
    KCmdLineLastOption
};

int main(int argc, char **argv)
{
    KAboutData about("$APPNAMELC$", I18N_NOOP("$APPNAME$"), version, description,
                     KAboutData::License_$LICENSE$, "(C) $YEAR$ $AUTHOR$", 0, 0, "$EMAIL$");
    about.addAuthor( "$AUTHOR$", 0, "$EMAIL$" );
    KCmdLineArgs::init(argc, argv, &about);
    KCmdLineArgs::addCmdLineOptions( options );
    KApplication app;
    $APPNAME$ *mainWin = 0;

    if (app.isRestored())
    {
        RESTORE($APPNAME$);
    }
    else
    {
        // no session.. just start up normally
        KCmdLineArgs *args = KCmdLineArgs::parsedArgs();

        // TODO: do something with the command line args here

        mainWin = new $APPNAME$();
        app.setMainWidget( mainWin );
        mainWin->show();

        args->clear();
    }

    int ret = app.exec();

    delete mainWin;
    return ret;
}
