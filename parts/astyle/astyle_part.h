/*
 *  Copyright (C) 2001 Matthias H�lzer-Kl�pfel <mhk@caldera.de>
 */


#ifndef __KDEVPART_ASTYLE_H__
#define __KDEVPART_ASTYLE_H__

class KDialogBase;
#include <kaction.h>
#include <kparts/part.h>
#include <kdevsourceformatter.h>


class AStylePart : public KDevSourceFormatter
{
  Q_OBJECT

public:

  AStylePart(QObject *parent, const char *name, const QStringList &);
  ~AStylePart();

  QString formatSource( const QString text );


private slots:

  void activePartChanged(KParts::Part *part);

  void beautifySource();

  void configWidget(KDialogBase *dlg);



private:

  void cursorPos( KParts::Part *part, uint * col, uint * line );
  void setCursorPos( KParts::Part *part, uint col, uint line );

  KAction *_action;

};


#endif
