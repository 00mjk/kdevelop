#ifndef __ASTYLE_WIDGET_H__
#define __ASTYLE_WIDGET_H__


#include <qlist.h>


#include "astyleconfig.h"


class KDevPart;


class AStyleWidget : public AStyleConfig
{
  Q_OBJECT
    
public:
		  
  AStyleWidget(QWidget *parent=0, const char *name=0);
  ~AStyleWidget();


public slots:
      
  void accept();


private slots:

  void styleChanged(int id);

};


#endif
