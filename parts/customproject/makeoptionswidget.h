/***************************************************************************
 *   Copyright (C) 2001 by Bernd Gehrmann                                  *
 *   bernd@kdevelop.org                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _MAKEOPTIONSWIDGET_H_
#define _MAKEOPTIONSWIDGET_H_

#include "makeoptionswidgetbase.h"

class CustomProjectPart;


class MakeOptionsWidget : public MakeOptionsWidgetBase
{
    Q_OBJECT
    
public:
    MakeOptionsWidget( CustomProjectPart *part, QWidget *parent=0, const char *name=0 );
    ~MakeOptionsWidget();

public slots:
    void accept();

private:
    CustomProjectPart *m_part;
};

#endif
